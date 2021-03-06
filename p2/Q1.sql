-- customer name, order number, and order size
SELECT C.C_NAME AS customer_name, O.O_ORDERKEY AS order_number, sum(L.L_QUANTITY * (L.L_EXTENDEDPRICE - L.L_DISCOUNT)) AS order_size
FROM ORDERS AS O, LINEITEM AS L, CUSTOMER AS C
WHERE L.L_ORDERKEY = O.O_ORDERKEY
AND O.O_CUSTKEY = C.C_CUSTKEY
GROUP BY O.O_ORDERKEY
ORDER BY order_size DESC
LIMIT 20