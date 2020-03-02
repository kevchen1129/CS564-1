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

/**
 * BTreeIndex Constructor.
 * Check to see if the corresponding index file exists. If so, open the file.
 * If not, create it and insert entries for every tuple in the base relation using FileScan class.
 *
 * The name of the relation on which to build the index.
 * The constructor should scan this relation (using ​FileScan​)
 * and insert entries for all the tuples in this relation into the index.
 * You can insert an entry one-by-one.
 *
 * The name of the index file; determine this name in the constructor as shown above,
 * and return the name.
 *
 * The instance of the global buffer manager.
 *
 * The byte offset of the attribute in the tuple on which to build the index.
 *
 * The data type of the attribute we are indexing.
 */
    BTreeIndex::BTreeIndex(const std::string &relationName,
                           std::string &outIndexName,
                           BufMgr *bufMgrIn,
                           const int attrByteOffset,
                           const Datatype attrType) {

        // default setting for building btree
        this->bufMgr = bufMgrIn; // (*this).bufMgr
        this->attributeType = attrType;                  // Datatype of attribute over which index is built
        this->attrByteOffset = attrByteOffset;      // Offset of attribute, over which index is built, inside records
        this->leafOccupancy = INTARRAYLEAFSIZE;      // Number of keys in leaf node, depending upon the type of key
        this->nodeOccupancy = INTARRAYNONLEAFSIZE;    // Number of keys in non-leaf node, depending upon the type of key

        // default setting for scanning btree
        scanExecuting = false;

        // an index file name is constructed as “​relName.attrOffset​”.
        // indexName is the name of the index file
        std::ostringstream idxStr;
        idxStr << relationName << '.' << attrByteOffset;
        outIndexName = idxStr.str();

        // Check whether the corresponding index file exists. If so, open the file. If not, create it.
        // we can determine if an index file already exists by seeing if that exception gets thrown
        // when you try to create the file with the create_new flag set to false,
        // if it does get thrown, you know that you need to create a new one.
        try {
            this->file = new BlobFile(outIndexName, false);
            std::cout << "BlobFile " << outIndexName << " exists.\n";

            // get header page of the index file check whether meta-data is correct
            this->headerPageNum = this->file->getFirstPageNo(); // Page number of meta page
            Page *headerPage;
            bufMgr->readPage(this->file, this->headerPageNum, headerPage);
            IndexMetaInfo *meta = (IndexMetaInfo *) headerPage;
            this->rootPageNum = meta->rootPageNo; // page number of root page of B+ tree inside index file

            // check whether meta-data match
            // std::cout << "relationName " << relationName << " " <<  meta->relationName[0] << "\n";
            // std::cout << "attrType " << attrType << " " <<  meta->attrType << "\n";
            // std::cout << "attrByteOffset " << attrByteOffset << " " <<  meta->attrByteOffset << "\n";
            if (relationName.compare(meta->relationName) != 0 || attrType != meta->attrType ||
                attrByteOffset != meta->attrByteOffset) {
                throw BadIndexInfoException(outIndexName);
            }

            // unpin headerpage from memory since it is no longer required to remain in memory
            // true if the page to be unpinned is updated
            bufMgr->unPinPage(file, headerPageNum, false);
        } catch (FileNotFoundException &e) {
            std::cout << "create BlobFile " << outIndexName << ".\n";
            this->file = new BlobFile(outIndexName, true);

            // dedicate a header page for storing meta-data of the index file
            // allocate header page
            Page *headerPage;
            bufMgr->allocPage(file, headerPageNum, headerPage);

            // build meta info
            IndexMetaInfo *meta = (IndexMetaInfo *) headerPage;
            meta->attrByteOffset = attrByteOffset;
            meta->attrType = attrType;
            strncpy((char *) (&(meta->relationName)), relationName.c_str(), 20);
            meta->relationName[19] = 0;

            // allocate root page
            Page *rootPage;
            bufMgr->allocPage(file, rootPageNum, rootPage);
            meta->rootPageNo = rootPageNum;
            initialPageNum = rootPageNum;
            LeafNodeInt *root = (LeafNodeInt *) rootPage;
            root->rightSibPageNo = 0;

            // unpin headerpage from memory since it is no longer required to remain in memory
            // true if the page to be unpinned is updated
            bufMgr->unPinPage(file, headerPageNum, true);
            bufMgr->unPinPage(file, rootPageNum, true);

            // the constructor should scan this relation (using ​FileScan​)
            // and insert entries for all the tuples in this relation into the index.
            FileScan fileScan(relationName, bufMgr);
            try {
                RecordId scanRid;
                while (1) {
                    fileScan.scanNext(scanRid);
                    std::string recordStr = fileScan.getRecord();
                    insertEntry(recordStr.c_str() + attrByteOffset, scanRid);
                }
            }
            catch (EndOfFileException e) {
                // save Btee index file to disk
                bufMgr->flushFile(file);
                std::cout << "Read all records" << std::endl;
            }
        }
    }

// -----------------------------------------------------------------------------
// BTreeIndex::~BTreeIndex -- destructor
// -----------------------------------------------------------------------------

/**
 * The destructor. Perform any cleanup that may be necessary, including clearing up any state variables, unpinning any B+ tree pages that are pinned, and flushing the index file (by calling the function bufMgr->flushFile()​). Note that this method does not delete the index file! But, deletion of the ​file object is required, which will call the destructor of ​File​ class causing the index file to be closed.
 */
    BTreeIndex::~BTreeIndex() {
        scanExecuting = false;
        this->bufMgr->flushFile(BTreeIndex::file);
        delete this->file;
        this->file = nullptr;
    }

// -----------------------------------------------------------------------------
// BTreeIndex::insertEntry
// -----------------------------------------------------------------------------

    const void BTreeIndex::insert(Page *curPage, PageId curPageNum, bool nodeIsLeaf, const RIDKeyPair<int> dataEntry,
                                  PageKeyPair<int> *&newchildEntry) {
        // nonleaf node
        if (!nodeIsLeaf) {
            NonLeafNodeInt *curNode = (NonLeafNodeInt *) curPage;
            // find the right key to traverse
            Page *nextPage;
            PageId nextNodeNum;
            findNextNonLeafNode(curNode, nextNodeNum, dataEntry.key);
            bufMgr->readPage(file, nextNodeNum, nextPage);
            nodeIsLeaf = curNode->level == 1;
            insert(nextPage, nextNodeNum, nodeIsLeaf, dataEntry, newchildEntry);

            // no split in child, just return
            if (newchildEntry == nullptr) {
                // unpin current page from call stack
                bufMgr->unPinPage(file, curPageNum, false);
            } else {
                // if the curpage is not full
                if (curNode->pageNoArray[nodeOccupancy] == 0) {
                    // insert the newchildEntry to curpage
                    insertNonLeaf(curNode, newchildEntry);
                    newchildEntry = nullptr;
                    // finish the insert process, unpin current page
                    bufMgr->unPinPage(file, curPageNum, true);
                } else {
                    splitNonLeaf(curNode, curPageNum, newchildEntry);
                }
            }
        } else {
            LeafNodeInt *leaf = (LeafNodeInt *) curPage;
            // page is not full
            if (leaf->ridArray[leafOccupancy - 1].page_number == 0) {
                insertLeaf(leaf, dataEntry);
                bufMgr->unPinPage(file, curPageNum, true);
                newchildEntry = nullptr;
            } else {
                splitLeaf(leaf, curPageNum, newchildEntry, dataEntry);
            }
        }
    }

    const void BTreeIndex::findNextNonLeafNode(NonLeafNodeInt *curNode, PageId &nextNodeNum, int key) {
        int i = nodeOccupancy;
        while (i >= 0 && (curNode->pageNoArray[i] == 0)) {
            i--;
        }
        while (i > 0 && (curNode->keyArray[i - 1] >= key)) {
            i--;
        }
        nextNodeNum = curNode->pageNoArray[i];
    }

    const void BTreeIndex::splitNonLeaf(NonLeafNodeInt *oldNode, PageId oldPageNum, PageKeyPair<int> *&newchildEntry) {
        // allocate a new non-leaf node
        PageId newPageNum;
        Page *newPage;
        bufMgr->allocPage(file, newPageNum, newPage);
        NonLeafNodeInt *newNode = (NonLeafNodeInt *) newPage;

        int mid = nodeOccupancy / 2;
        int pushupIndex = mid;
        PageKeyPair<int> pushupEntry;
        // even number of keys
        if (nodeOccupancy % 2 == 0) {
            pushupIndex = newchildEntry->key < oldNode->keyArray[mid] ? mid - 1 : mid;
        }
        pushupEntry.set(newPageNum, oldNode->keyArray[pushupIndex]);
//?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
        mid = pushupIndex + 1;
        // move half the entries to the new node
        for (int i = mid; i < nodeOccupancy; i++) {
            newNode->keyArray[i - mid] = oldNode->keyArray[i];
            newNode->pageNoArray[i - mid] = oldNode->pageNoArray[i + 1];
            oldNode->pageNoArray[i + 1] = (PageId) 0;
            oldNode->keyArray[i] = 0;
        }
//?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
        newNode->level = oldNode->level;
        // remove the entry that is pushed up from current node
        oldNode->keyArray[pushupIndex] = 0;
        oldNode->pageNoArray[pushupIndex] = (PageId) 0;
        // insert the new child entry
        insertNonLeaf(newchildEntry->key < newNode->keyArray[0] ? oldNode : newNode, newchildEntry);
        // newchildEntry = new PageKeyPair<int>();
        newchildEntry = &pushupEntry;
        bufMgr->unPinPage(file, oldPageNum, true);
        bufMgr->unPinPage(file, newPageNum, true);

        // if the curNode is the root
        if (oldPageNum == rootPageNum) {
            updateRoot(oldPageNum, newchildEntry);
        }
    }

    const void BTreeIndex::insertNonLeaf(NonLeafNodeInt *nonleaf, PageKeyPair<int> *entry) {

        int i = nodeOccupancy;
        while (i >= 0 && (nonleaf->pageNoArray[i] == 0)) {
            i--;
        }
        // shift
        while (i > 0 && (nonleaf->keyArray[i - 1] > entry->key)) {
            nonleaf->keyArray[i] = nonleaf->keyArray[i - 1];
            nonleaf->pageNoArray[i + 1] = nonleaf->pageNoArray[i];
            i--;
        }
        // insert
        nonleaf->keyArray[i] = entry->key;
        nonleaf->pageNoArray[i + 1] = entry->pageNo;
    }

    const void BTreeIndex::splitLeaf(LeafNodeInt *leaf, PageId leafPageNum, PageKeyPair<int> *&newchildEntry,
                                     const RIDKeyPair<int> dataEntry) {
        // allocate a new leaf page
        PageId newPageNum;
        Page *newPage;
        bufMgr->allocPage(file, newPageNum, newPage);
        LeafNodeInt *newLeafNode = (LeafNodeInt *) newPage;

        int mid = leafOccupancy / 2;
        // odd number of keys
        if (leafOccupancy % 2 == 1 && dataEntry.key > leaf->keyArray[mid]) {
            mid = mid + 1;
        }
        // copy half the page to newLeafNode
        for (int i = mid; i < leafOccupancy; i++) {
            newLeafNode->keyArray[i - mid] = leaf->keyArray[i];
            newLeafNode->ridArray[i - mid] = leaf->ridArray[i];
            leaf->keyArray[i] = 0;
            leaf->ridArray[i].page_number = 0;
        }

        if (dataEntry.key > leaf->keyArray[mid - 1]) {
            insertLeaf(newLeafNode, dataEntry);
        } else {
            insertLeaf(leaf, dataEntry);
        }

        // update sibling pointer
        newLeafNode->rightSibPageNo = leaf->rightSibPageNo;
        leaf->rightSibPageNo = newPageNum;

        // the smallest key from second page as the new child entry
        newchildEntry = new PageKeyPair<int>();
        PageKeyPair<int> newKeyPair;
        newKeyPair.set(newPageNum, newLeafNode->keyArray[0]);
        newchildEntry = &newKeyPair;
        bufMgr->unPinPage(file, leafPageNum, true);
        bufMgr->unPinPage(file, newPageNum, true);

        // if curr page is root
        if (leafPageNum == rootPageNum) {
            updateRoot(leafPageNum, newchildEntry);
        }
    }

    const void BTreeIndex::insertLeaf(LeafNodeInt *leaf, RIDKeyPair<int> entry) {
        // empty leaf page
        if (leaf->ridArray[0].page_number == 0) {
            leaf->keyArray[0] = entry.key;
            leaf->ridArray[0] = entry.rid;
        } else {
            int i = leafOccupancy - 1;
            // find the end
            while (i >= 0 && (leaf->ridArray[i].page_number == 0)) {
                i--;
            }
            // shift entry
            while (i >= 0 && (leaf->keyArray[i] > entry.key)) {
                leaf->keyArray[i + 1] = leaf->keyArray[i];
                leaf->ridArray[i + 1] = leaf->ridArray[i];
                i--;
            }
            // insert entry
            leaf->keyArray[i + 1] = entry.key;
            leaf->ridArray[i + 1] = entry.rid;
        }
    }

    const void BTreeIndex::updateRoot(PageId firstPageInRoot, PageKeyPair<int> *newchildEntry) {
        // create a new root
        PageId newRootPageNum;
        Page *newRoot;
        bufMgr->allocPage(file, newRootPageNum, newRoot);
        NonLeafNodeInt *newRootPage = (NonLeafNodeInt *) newRoot;

        // update metadata
        newRootPage->level = initialPageNum == rootPageNum ? 1 : 0;
        newRootPage->pageNoArray[0] = firstPageInRoot;
        newRootPage->pageNoArray[1] = newchildEntry->pageNo;
        newRootPage->keyArray[0] = newchildEntry->key;

        Page *meta;
        bufMgr->readPage(file, headerPageNum, meta);
        IndexMetaInfo *metaPage = (IndexMetaInfo *) meta;
        metaPage->rootPageNo = newRootPageNum;
        rootPageNum = newRootPageNum;
        // unpin unused page
        bufMgr->unPinPage(file, headerPageNum, true);
        bufMgr->unPinPage(file, newRootPageNum, true);
    }

/**
 * This method inserts a new entry into the index using the pair <key, rid>.
 * const void *key: pointer to the value (integer) we want to insert.
 * const RecordId rid: the corresponding record id of the tuple in the base relation.
 */
    const void BTreeIndex::insertEntry(const void *key, const RecordId rid) {
        // std::cout << "page_number: " << rid.page_number << ", slot_number: " << rid.slot_number << "key: " << key << std::endl;
        RIDKeyPair<int> dataEntry;
        dataEntry.set(rid, *((int *) key));
        // root
        Page *root;
        // PageId rootPageNum;
        bufMgr->readPage(file, rootPageNum, root);
        PageKeyPair<int> *childEntry = nullptr;
        insert(root, rootPageNum, initialPageNum == rootPageNum ? true : false, dataEntry, childEntry);
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

        // Scan is already started
        if (scanExecuting) {
            endScan();
        }

        currentPageNum = rootPageNum;
        // Start scanning by reading rootpage into the buffer pool
        bufMgr->readPage(file, currentPageNum, currentPageData);

        // root is not a leaf
        if (initialPageNum != rootPageNum) {
            // Cast
            NonLeafNodeInt *currentNode = (NonLeafNodeInt *) currentPageData;
            bool foundLeaf = false;
            while (!foundLeaf) {
                // Cast page to node
                currentNode = (NonLeafNodeInt *) currentPageData;
                // Check if this is the level above the leaf, if yes, the next level is the leaf
                if (currentNode->level == 1) {
                    foundLeaf = true;
                }

                // Find the leaf
                PageId nextPageNum;
                findNextNonLeafNode(currentNode, nextPageNum, lowValInt);
                // Unpin
                bufMgr->unPinPage(file, currentPageNum, false);
                currentPageNum = nextPageNum;
                // read the nextPage
                bufMgr->readPage(file, currentPageNum, currentPageData);
            }
        }
        // Now the curNode is leaf node try to find the smallest one that satisefy the OP
        bool found = false;
        while (!found) {
            // Cast page to node
            LeafNodeInt *currentNode = (LeafNodeInt *) currentPageData;
            // Check if the whole page is null
            if (currentNode->ridArray[0].page_number == 0) {
                bufMgr->unPinPage(file, currentPageNum, false);
                throw NoSuchKeyFoundException();
            }
            // Search from the left leaf page to the right to find the fit
            bool nullVal = false;
            for (int i = 0; i < leafOccupancy and !nullVal; i++) {
                int key = currentNode->keyArray[i];
                // Check if the next one in the key is not inserted
                if (i < leafOccupancy - 1 and currentNode->ridArray[i + 1].page_number == 0) {
                    nullVal = true;
                }

                if (checkKey(lowValInt, lowOp, highValInt, highOp, key)) {
                    // select
                    nextEntry = i;
                    found = true;
                    scanExecuting = true;
                    break;
                } else if ((highOp == LT and key >= highValInt) or (highOp == LTE and key > highValInt)) {
                    bufMgr->unPinPage(file, currentPageNum, false);
                    throw NoSuchKeyFoundException();
                }

                // Did not find any matching key in this leaf, go to next leaf
                if (i == leafOccupancy - 1 or nullVal) {
                    //unpin page
                    bufMgr->unPinPage(file, currentPageNum, false);
                    //did not find the matching one in the more right leaf
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
        // Cast page to node
        LeafNodeInt *currentNode = (LeafNodeInt *) currentPageData;
        if (currentNode->ridArray[nextEntry].page_number == 0 or nextEntry == leafOccupancy) {
            // Unpin page and read papge
            bufMgr->unPinPage(file, currentPageNum, false);
            // No more next leaf
//////////change/////////////change
            if (currentNode->rightSibPageNo == 0) {
                throw IndexScanCompletedException();
            }
            currentPageNum = currentNode->rightSibPageNo;
//////////change //////////change
            bufMgr->readPage(file, currentPageNum, currentPageData);
            currentNode = (LeafNodeInt *) currentPageData;
            // Reset nextEntry
            nextEntry = 0;
        }

        // Check  if rid satisfy
        int key = currentNode->keyArray[nextEntry];
        if (checkKey(lowValInt, lowOp, highValInt, highOp, key)) {
            outRid = currentNode->ridArray[nextEntry];
            // Increment nextEntry
            nextEntry++;
            // If current page has been scanned to its entirety
        } else {
            throw IndexScanCompletedException();
        }
    }

// -----------------------------------------------------------------------------
// BTreeIndex::endScan
// -----------------------------------------------------------------------------
//
    const void BTreeIndex::endScan() {
        if (!scanExecuting) {
            throw ScanNotInitializedException();
        }
        scanExecuting = false;
        // Unpin page
        bufMgr->unPinPage(file, currentPageNum, false);
        // Reset variable
        currentPageData = nullptr;
        currentPageNum = static_cast<PageId>(-1);
        nextEntry = -1;
    }

    const bool BTreeIndex::checkKey(int lowVal, const Operator lowOp, int highVal, const Operator highOp, int key) {
        if (lowOp == GTE && highOp == LTE) {
            return key <= highVal && key >= lowVal;
        } else if (lowOp == GT && highOp == LTE) {
            return key <= highVal && key > lowVal;
        } else if (lowOp == GTE && highOp == LT) {
            return key < highVal && key >= lowVal;
        } else {
            return key < highVal && key > lowVal;
        }
    }

}

