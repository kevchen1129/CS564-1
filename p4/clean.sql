-- Query2_opt

drop table if exists nationWithRegion;
drop INDEX if exists Q2Idx1;
drop INDEX if exists Q2Idx2;
drop INDEX if exists Q2Idx3;

------------------------------------------------------------------
-- Query3_opt

drop INDEX if exists Q3Idx1;
drop INDEX if exists Q3Idx2;
drop INDEX if exists Q3Idx3;

------------------------------------------------------------------
-- Query4_opt

drop INDEX if exists Q4Idx1;
drop INDEX if exists Q4Idx2;
drop INDEX if exists Q4Idx3;

------------------------------------------------------------------

-- drop INDEX  if exists part_index;
-- drop INDEX  if exists part_index_size;
-- drop INDEX  if exists part_index_type;

-- drop INDEX  if exists supplier_index;
-- drop INDEX  if exists supplier_index_name;
-- drop INDEX  if exists supplier_index_suppkey;

-- drop INDEX  if exists partsupp_index;
-- drop INDEX  if exists partsupp_index_supplycost;

-- drop INDEX  if exists nation_index;
-- drop INDEX  if exists nation_index_nationkey;

-- drop INDEX  if exists region_index;
-- drop INDEX  if exists region_index_regionkey;

-- drop INDEX  if exists customer_index;
-- drop INDEX  if exists customer_index_mk;

-- drop INDEX  if exists orders_index;
-- drop INDEX  if exists orders_index_custkey;
-- drop INDEX  if exists orders_index_orderdate;

-- drop INDEX  if exists lineitem_index;
-- drop INDEX  if exists lineitem_index_shipdate;