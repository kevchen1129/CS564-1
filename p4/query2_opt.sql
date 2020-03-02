.timer on

SELECT 
	s_acctbal, s_name, NationName as N_NAME, p_partkey, p_mfgr, s_address, s_phone, s_comment
FROM 
	part, supplier, partsupp, nationWithRegion
WHERE 
	p_partkey = ps_partkey
	AND s_suppkey = ps_suppkey
	AND p_size = 11 -- [SIZE]
	AND p_type like 'MEDIUM BRUSHED COPPER' -- '%[TYPE]'
	AND s_nationkey = W_NATIONKEY
	AND RegionName = 'ASIA' -- '[REGION]'
	AND ps_supplycost = (
		SELECT
			min(ps_supplycost) 
		FROM
			partsupp, supplier, nationWithRegion 
		WHERE
			p_partkey = ps_partkey
			AND s_suppkey = ps_suppkey
			AND s_nationkey = W_NATIONKEY
			AND RegionName = 'ASIA' -- '[REGION]'
		)
ORDER BY
	s_acctbal DESC,
	NationName,
	s_name,
	p_partkey;
 