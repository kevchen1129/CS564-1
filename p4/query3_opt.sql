.timer on
select
	l_orderkey,
	sum(l_extendedprice*(1-l_discount)) as revenue,
	o_orderdate,
	o_shippriority
from
	(select c_custkey from customer where c_mktsegment = 'BUILDING') as t1,
	(select o_orderdate, o_shippriority, o_custkey, o_orderkey from orders where o_orderdate < "1995-03-15") as t2,
	(select l_orderkey, l_extendedprice, l_discount from lineitem where l_shipdate > "1995-03-15") as t3
where
	c_custkey = o_custkey
	and l_orderkey = o_orderkey
group by
	l_orderkey,
	o_orderdate,
	o_shippriority
order by
	revenue desc,
	o_orderdate;