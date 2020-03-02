/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "btree.h"
#include "filescan.h"
#include "exceptions/bad_index_info_exception.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/end_of_file_exception.h"

//#define DEBUG

namespace badgerdb {

// -----------------------------------------------------------------------------
// BTreeIndex::BTreeIndex -- Constructor
// -----------------------------------------------------------------------------
    BTreeIndex::BTreeIndex(const std::string &relationName,
                           std::string &outIndexName,
                           BufMgr *bufMgrIn,
                           const int attrByteOffset,
                           const Datatype attrType) {
        try {
            scanExecuting = false;
            std::ostringstream idxStr;
            idxStr << relationName << '.' << attrByteOffset;
            outIndexName = idxStr.str();

            leafOccupancy = INTARRAYLEAFSIZE;
            nodeOccupancy = INTARRAYNONLEAFSIZE;
            bufMgr = bufMgrIn;
            attributeType = attrType;
            this->attrByteOffset = attrByteOffset;
            file = new BlobFile(outIndexName, false);
            std::cout << "BlobFile " << outIndexName << " exists.\n";

            headerPageNum = (*file).getFirstPageNo();
            Page *headerPage;
            bufMgr->readPage(file, headerPageNum, headerPage);
            IndexMetaInfo *metaData = (IndexMetaInfo *) headerPage;
            this->rootPageNum = metaData->rootPageNo;

            if (relationName.compare(metaData->relationName) != 0 ||
                metaData->attrByteOffset != attrByteOffset || metaData->attrType != attrType) {
                throw BadIndexInfoException(outIndexName);
            }

            bufMgr->unPinPage(file, headerPageNum, false);
        } catch (FileNotFoundException &e) {
            std::cout << "create BlobFile " << outIndexName << ".\n";
            file = new BlobFile(outIndexName, true);

            Page *headerPage;
            bufMgr->allocPage(file, headerPageNum, headerPage);

            IndexMetaInfo *metaData = (IndexMetaInfo *) headerPage;
            metaData->attrByteOffset = attrByteOffset;
            metaData->attrType = attrType;
            strncpy((char *) (&(metaData->relationName)), relationName.c_str(), 20);
            metaData->relationName[19] = 0;

            Page *rootPage;
            bufMgr->allocPage(file, rootPageNum, rootPage);
            metaData->rootPageNo = rootPageNum;
            initialPageNum = rootPageNum;
            LeafNodeInt *root = (LeafNodeInt *) rootPage;
            root->rightSibPageNo = 0;
            bufMgr->unPinPage(file, headerPageNum, true);
            bufMgr->unPinPage(file, rootPageNum, true);
            FileScan fileScan(relationName, bufMgr);
            RecordId RID;
            try {
                while (true) {
                    fileScan.scanNext(RID);
                    std::string record = fileScan.getRecord();
                    insertEntry(record.c_str() + attrByteOffset, RID);
                }
            }
            catch (EndOfFileException e) {
                bufMgr->flushFile(file);
            }
        }
    }

// -----------------------------------------------------------------------------
// BTreeIndex::~BTreeIndex -- destructor
// -----------------------------------------------------------------------------
    BTreeIndex::~BTreeIndex() {
        scanExecuting = false;
        this->bufMgr->flushFile(BTreeIndex::file);
        delete this->file;
        this->file = nullptr;
    }

// -----------------------------------------------------------------------------
// BTreeIndex::insertEntry
// -----------------------------------------------------------------------------
    const void
    BTreeIndex::insertRecur(Page *curPage, PageId curPageNum, bool nodeIsLeaf, const RIDKeyPair<int> dataEntry,
                            PageKeyPair<int> *&newChildEntry) {
        if (!nodeIsLeaf) {
            NonLeafNodeInt *curNode = (NonLeafNodeInt *) curPage;
            Page *nextPage;
            PageId nextNode;

            int index = nodeOccupancy;
            while (index >= 0 && (curNode->pageNoArray[index] == 0)) {
                index--;
            }
            while (index > 0 && (curNode->keyArray[index - 1] >= dataEntry.key)) {
                index--;
            }
            nextNode = curNode->pageNoArray[index];

            bufMgr->readPage(file, nextNode, nextPage);
            nodeIsLeaf = curNode->level == 1;
            insertRecur(nextPage, nextNode, nodeIsLeaf, dataEntry, newChildEntry);
            if (newChildEntry == nullptr) {
                bufMgr->unPinPage(file, curPageNum, false);
            } else {
                if (curNode->pageNoArray[nodeOccupancy] == 0) {
                    int index = nodeOccupancy;
                    while (index >= 0 && (curNode->pageNoArray[index] == 0)) {
                        index--;
                    }
                    while (index > 0 && (curNode->keyArray[index - 1] > newChildEntry->key)) {
                        curNode->keyArray[index] = curNode->keyArray[index - 1];
                        curNode->pageNoArray[index + 1] = curNode->pageNoArray[index];
                        index--;
                    }
                    curNode->keyArray[index] = newChildEntry->key;
                    curNode->pageNoArray[index + 1] = newChildEntry->pageNo;

                    newChildEntry = nullptr;
                    bufMgr->unPinPage(file, curPageNum, true);
                } else {
// split Non-Leaf-------------------------------------------------------------------------------------------------------
                    PageId newPageId;
                    Page *newPage;
                    bufMgr->allocPage(file, newPageId, newPage);
                    NonLeafNodeInt *newNode = (NonLeafNodeInt *) newPage;
                    int middle = nodeOccupancy / 2;
                    int index = middle;
                    PageKeyPair<int> p;
                    if (nodeOccupancy % 2 == 0) {
                        index = newChildEntry->key < curNode->keyArray[middle] ? middle - 1 : middle;
                    }
                    p.set(newPageId, curNode->keyArray[index]);
                    middle = index + 1;
////???????/???????/???????/???????/???????////???????/???????/???????/??????
                    for (int i = middle; i < nodeOccupancy; i++) {
                        newNode->keyArray[i - middle] = curNode->keyArray[i];
                        newNode->pageNoArray[i - middle] = curNode->pageNoArray[i + 1];
                        curNode->pageNoArray[i + 1] = (PageId) 0;
                        curNode->keyArray[i] = 0;
                    }
////???????/???????/???????/??????//???????////???????/???????/???????/??????
                    newNode->level = curNode->level;
                    curNode->keyArray[index] = 0;
                    curNode->pageNoArray[index] = (PageId) 0;

                    NonLeafNodeInt *nonLeafNode = newChildEntry->key < newNode->keyArray[0] ? curNode : newNode;

                    int index2 = nodeOccupancy;
                    while (index2 >= 0 && (nonLeafNode->pageNoArray[index2] == 0)) {
                        index2--;
                    }
                    while (index2 > 0 && (nonLeafNode->keyArray[index2 - 1] > newChildEntry->key)) {
                        nonLeafNode->keyArray[index2] = nonLeafNode->keyArray[index2 - 1];
                        nonLeafNode->pageNoArray[index2 + 1] = nonLeafNode->pageNoArray[index2];
                        index2--;
                    }
                    nonLeafNode->keyArray[index2] = newChildEntry->key;
                    nonLeafNode->pageNoArray[index2 + 1] = newChildEntry->pageNo;
/// end split----------------------------------------------------------------------------------------------
                    newChildEntry = &p;
                    bufMgr->unPinPage(file, curPageNum, true);
                    bufMgr->unPinPage(file, newPageId, true);
                    if (curPageNum == rootPageNum)
                    {
                        PageId newRootPageNum;
                        Page *newRoot;
                        bufMgr->allocPage(file, newRootPageNum, newRoot);
                        NonLeafNodeInt *newRootPage = (NonLeafNodeInt *) newRoot;
                        newRootPage->level = initialPageNum == rootPageNum ? 1 : 0;
                        newRootPage->pageNoArray[0] = curPageNum;
                        newRootPage->pageNoArray[1] = newChildEntry->pageNo;
                        newRootPage->keyArray[0] = newChildEntry->key;

                        Page *meta;
                        bufMgr->readPage(file, headerPageNum, meta);
                        IndexMetaInfo *metaPage = (IndexMetaInfo *) meta;
                        metaPage->rootPageNo = newRootPageNum;
                        rootPageNum = newRootPageNum;

                        bufMgr->unPinPage(file, headerPageNum, true);
                        bufMgr->unPinPage(file, newRootPageNum, true);
                    }
                }
            }
        } else {
            LeafNodeInt *leafNode = (LeafNodeInt *) curPage;
            if (leafNode->ridArray[leafOccupancy - 1].page_number == 0) {
                if (leafNode->ridArray[0].page_number == 0) {
                    leafNode->keyArray[0] = dataEntry.key;
                    leafNode->ridArray[0] = dataEntry.rid;
                } else {
                    int index = leafOccupancy - 1;
                    while (index >= 0 && (leafNode->ridArray[index].page_number == 0)) {
                        index--;
                    }
                    while (index >= 0 && (leafNode->keyArray[index] > dataEntry.key)) {
                        leafNode->keyArray[index + 1] = leafNode->keyArray[index];
                        leafNode->ridArray[index + 1] = leafNode->ridArray[index];
                        index--;
                    }
                    leafNode->keyArray[index + 1] = dataEntry.key;
                    leafNode->ridArray[index + 1] = dataEntry.rid;
                }
                bufMgr->unPinPage(file, curPageNum, true);
                newChildEntry = nullptr;
            } else {
                PageId newPageNum;
                Page *newPage;
                bufMgr->allocPage(file, newPageNum, newPage);
                LeafNodeInt *newLeafNode = (LeafNodeInt *) newPage;

                int middle = leafOccupancy / 2;
                if (leafOccupancy % 2 == 1 && dataEntry.key > leafNode->keyArray[middle]) {
                    middle += 1;
                }
                for (int i = middle; i < leafOccupancy; i++) {
                    newLeafNode->keyArray[i - middle] = leafNode->keyArray[i];
                    newLeafNode->ridArray[i - middle] = leafNode->ridArray[i];
                    leafNode->keyArray[i] = 0;
                    leafNode->ridArray[i].page_number = 0;
                }

                if (dataEntry.key > leafNode->keyArray[middle - 1]) {
                    if (newLeafNode->ridArray[0].page_number == 0) {
                        newLeafNode->keyArray[0] = dataEntry.key;
                        newLeafNode->ridArray[0] = dataEntry.rid;
                    } else {
                        int index = leafOccupancy - 1;
                        while (index >= 0 && (leafNode->ridArray[index].page_number == 0)) {
                            index--;
                        }
                        while (index >= 0 && (leafNode->keyArray[index] > dataEntry.key)) {
                            newLeafNode->keyArray[index + 1] = leafNode->keyArray[index];
                            newLeafNode->ridArray[index + 1] = leafNode->ridArray[index];
                            index--;
                        }
                        newLeafNode->keyArray[index + 1] = dataEntry.key;
                        newLeafNode->ridArray[index + 1] = dataEntry.rid;
                    }

                } else {
                    if (leafNode->ridArray[0].page_number == 0) {
                        leafNode->keyArray[0] = dataEntry.key;
                        leafNode->ridArray[0] = dataEntry.rid;
                    } else {
                        int index = leafOccupancy - 1;
                        while (index >= 0 && (leafNode->ridArray[index].page_number == 0)) {
                            index--;
                        }
                        while (index >= 0 && (leafNode->keyArray[index] > dataEntry.key)) {
                            leafNode->keyArray[index + 1] = leafNode->keyArray[index];
                            leafNode->ridArray[index + 1] = leafNode->ridArray[index];
                            index--;
                        }
                        leafNode->keyArray[index + 1] = dataEntry.key;
                        leafNode->ridArray[index + 1] = dataEntry.rid;
                    }
                }
                newLeafNode->rightSibPageNo = leafNode->rightSibPageNo;
                leafNode->rightSibPageNo = newPageNum;
                newChildEntry = new PageKeyPair<int>();
                PageKeyPair<int> newKey;
                newKey.set(newPageNum, newLeafNode->keyArray[0]);
                newChildEntry = &newKey;
                bufMgr->unPinPage(file, curPageNum, true);
                bufMgr->unPinPage(file, newPageNum, true);
                if (curPageNum == rootPageNum) {
                    PageId newRootPageNum;
                    Page *newRoot;
                    bufMgr->allocPage(file, newRootPageNum, newRoot);
                    NonLeafNodeInt *newPage = (NonLeafNodeInt *) newRoot;

                    newPage->level = initialPageNum == rootPageNum ? 1 : 0;
                    newPage->pageNoArray[0] = curPageNum;
                    newPage->pageNoArray[1] = newChildEntry->pageNo;
                    newPage->keyArray[0] = newChildEntry->key;

                    Page *meta;
                    bufMgr->readPage(file, headerPageNum, meta);
                    IndexMetaInfo *metaPage = (IndexMetaInfo *) meta;
                    metaPage->rootPageNo = newRootPageNum;
                    rootPageNum = newRootPageNum;

                    bufMgr->unPinPage(file, headerPageNum, true);
                    bufMgr->unPinPage(file, newRootPageNum, true);
                }
            }
        }
    }

    const void BTreeIndex::insertEntry(const void *key, const RecordId rid) {
        RIDKeyPair<int> dataEntry;
        dataEntry.set(rid, *((int *) key));
        Page *root;
        bufMgr->readPage(file, rootPageNum, root);
        PageKeyPair<int> *childEntry = nullptr;
        insertRecur(root, rootPageNum, initialPageNum == rootPageNum ? true : false, dataEntry, childEntry);
    }

// -----------------------------------------------------------------------------
// BTreeIndex::startScan
// -----------------------------------------------------------------------------

    const void BTreeIndex::startScan(const void *lowValParm,
                                     const Operator lowOpParm,
                                     const void *highValParm,
                                     const Operator highOpParm) {
        lowValInt = *((int *) lowValParm);
        highValInt = *((int *) highValParm);

        if (!((lowOpParm == GT or lowOpParm == GTE) and (highOpParm == LT or highOpParm == LTE))) {
            throw BadOpcodesException();
        }
        if (lowValInt > highValInt) {
            throw BadScanrangeException();
        }
        lowOp = lowOpParm;
        highOp = highOpParm;
        if (scanExecuting) {
            endScan();
        }
        currentPageNum = rootPageNum;
        bufMgr->readPage(file, currentPageNum, currentPageData);

        if (initialPageNum != rootPageNum) {
            NonLeafNodeInt *currentNode = (NonLeafNodeInt *) currentPageData;
            bool foundLeaf = false;
            while (!foundLeaf) {
                currentNode = (NonLeafNodeInt *) currentPageData;
                if (currentNode->level == 1) {
                    foundLeaf = true;
                }
                PageId nextPageNum;
                int index = nodeOccupancy;
                while (index >= 0 && (currentNode->pageNoArray[index] == 0)) {
                    index--;
                }
                while (index > 0 && (currentNode->keyArray[index - 1] >= lowValInt)) {
                    index--;
                }
                nextPageNum = currentNode->pageNoArray[index];
                bufMgr->unPinPage(file, currentPageNum, false);
                currentPageNum = nextPageNum;
                bufMgr->readPage(file, currentPageNum, currentPageData);
            }
        }
        bool found = false;
        while (!found) {
            LeafNodeInt *currentNode = (LeafNodeInt *) currentPageData;
            if (currentNode->ridArray[0].page_number == 0) {
                bufMgr->unPinPage(file, currentPageNum, false);
                throw NoSuchKeyFoundException();
            }
            bool n = false;
            for (int index = 0; index < leafOccupancy and !n; index++) {
                int num = currentNode->keyArray[index];
                if (index < leafOccupancy - 1 and currentNode->ridArray[index + 1].page_number == 0) {
                    n = true;
                }
                bool check;
                if (lowOp == GTE && highOp == LTE) {
                    check = num >= lowValInt && num <= highValInt;
                }
                if (lowOp == GT && highOp == LTE) {
                    check = num > lowValInt && num <= highValInt;
                }
                if (lowOp == GTE && highOp == LT) {
                    check = num >= lowValInt && num < highValInt;
                }
                if (lowOp == GT && highOp == LT) {
                    check = num > lowValInt && num < highValInt;
                }
                if (check) {
                    nextEntry = index;
                    found = true;
                    scanExecuting = true;
                    break;
                } else if ((num >= highValInt and highOp == LT) or (num > highValInt and highOp == LTE)) {
                    bufMgr->unPinPage(file, currentPageNum, false);
                    throw NoSuchKeyFoundException();
                }
                if (index == leafOccupancy - 1 or n) {
                    bufMgr->unPinPage(file, currentPageNum, false);
                    if (currentNode->rightSibPageNo == 0) {
                        throw NoSuchKeyFoundException();
                    }
                    currentPageNum = currentNode->rightSibPageNo;
                    bufMgr->readPage(file, currentPageNum, currentPageData);
                }
            }
        }
    }

// -----------------------------------------------------------------------------
// BTreeIndex::scanNext
// -----------------------------------------------------------------------------

    const void BTreeIndex::scanNext(RecordId &outRid) {
        if (!scanExecuting) {
            throw ScanNotInitializedException();
        }
        LeafNodeInt *node = (LeafNodeInt *) currentPageData;
        if (node->ridArray[nextEntry].page_number == 0 or nextEntry == leafOccupancy) {
            if (node->rightSibPageNo == 0) {
                throw IndexScanCompletedException();
            }
            bufMgr->unPinPage(file, currentPageNum, false);
            currentPageNum = node->rightSibPageNo;
            bufMgr->readPage(file, currentPageNum, currentPageData);
            node = (LeafNodeInt *) currentPageData;
            nextEntry = 0;
        }
        int num = node->keyArray[nextEntry];
        bool check;

        if (lowOp == GTE && highOp == LTE) {
            check = num >= lowValInt && num <= highValInt;
        }
        if (lowOp == GT && highOp == LTE) {
            check = num > lowValInt && num <= highValInt;
        }
        if (lowOp == GTE && highOp == LT) {
            check = num >= lowValInt && num < highValInt;
        }
        if (lowOp == GT && highOp == LT) {
            check = num > lowValInt && num < highValInt;
        }
        if (check) {
            outRid = node->ridArray[nextEntry];
            nextEntry++;
        } else {
            throw IndexScanCompletedException();
        }
    }

// -----------------------------------------------------------------------------
// BTreeIndex::endScan
// -----------------------------------------------------------------------------
    const void BTreeIndex::endScan() {
        if (!scanExecuting) {
            throw ScanNotInitializedException();
        }
        scanExecuting = false;
        bufMgr->unPinPage(file, currentPageNum, false);
        currentPageData = nullptr;
        currentPageNum = static_cast<PageId>(-1);
        nextEntry = -1;
    }
}
