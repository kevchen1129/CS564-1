-- Query2_opt

drop table if exists nationWithRegion;
drop INDEX if exists Q2Idx1;
drop INDEX if exists Q2Idx2;
drop INDEX if exists Q2Idx3;

CREATE TABLE		nationWithRegion
AS 
SELECT	N_NATIONKEY as W_NATIONKEY , R_NAME as RegionName, N_NAME as NationName
FROM		NATION, REGION				
WHERE	N_REGIONKEY = R_REGIONKEY;

CREATE INDEX Q2Idx1 ON PART(p_size);
-- CREATE INDEX Q2Idx2 ON ORDERS(o_orderkey, o_shippriority);
-- CREATE INDEX Q2Idx3 ON CUSTOMER(c_custkey, c_mktsegment);

------------------------------------------------------------------
-- Query3_opt

drop INDEX if exists Q3Idx1;
drop INDEX if exists Q3Idx2;
drop INDEX if exists Q3Idx3;

CREATE INDEX Q3Idx1 ON LINEITEM(l_orderkey, l_shipdate);
-- CREATE INDEX Q3Idx2 ON ORDERS(o_orderkey, o_shippriority);
-- CREATE INDEX Q3Idx3 ON CUSTOMER(c_custkey, c_mktsegment);

------------------------------------------------------------------
-- Query4_opt
drop INDEX if exists Q4Idx1;
drop INDEX if exists Q4Idx2;
drop INDEX if exists Q4Idx3;

-- CREATE INDEX Q4Idx1 ON LINEITEM(l_orderkey);
CREATE INDEX Q4Idx2 ON ORDERS(o_orderdate);
-- CREATE INDEX Q4Idx3 ON CUSTOMER(c_custkey, c_mktsegment);

------------------------------------------------------------------

-- CREATE INDEX part_index on part(p_partkey); --x
-- CREATE INDEX part_index_size on part(p_size); --o
-- CREATE INDEX part_index_type on part(p_type); --x

-- CREATE INDEX supplier_index on supplier(s_acctbal DESC); --x
-- CREATE INDEX supplier_index_name on supplier(s_name); --x
-- CREATE INDEX supplier_index_suppkey on supplier(s_suppkey); --x

-- CREATE INDEX partsupp_index on partsupp(ps_partkey, ps_suppkey); --x
-- CREATE INDEX partsupp_index_supplycost on partsupp(ps_supplycost); --x

-- CREATE INDEX nation_index on nation(n_name); --x
-- CREATE INDEX nation_index_nationkey on nation(n_nationkey); --x

-- CREATE INDEX region_index on region(r_name); --x
-- CREATE INDEX region_index_regionkey on region(r_regionkey); --x

-- CREATE INDEX customer_index on customer(c_custkey); --x
-- CREATE INDEX customer_index_mk on customer(c_mktsegment); --x

-- CREATE INDEX orders_index on orders(o_orderkey); --x
-- CREATE INDEX orders_index_custkey on orders(o_custkey); --x
-- CREATE INDEX orders_index_orderdate on orders(o_orderdate); --x

-- CREATE INDEX lineitem_index on lineitem(l_orderkey); --x
-- CREATE INDEX lineitem_index_shipdate on lineitem(l_shipdate); --x



