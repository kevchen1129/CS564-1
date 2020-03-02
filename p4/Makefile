all:
	sqlite3 TPC-H.db < preprocessing.sql; sqlite3 TPC-H.db < query2.sql; sqlite3 TPC-H.db < clean.sql;

op2:
	sqlite3 TPC-H.db < preprocessing.sql; sqlite3 TPC-H.db < query2_opt.sql; sqlite3 TPC-H.db < clean.sql;

op3:
	sqlite3 TPC-H.db < preprocessing.sql; sqlite3 TPC-H.db < query3_opt.sql; sqlite3 TPC-H.db < clean.sql;

op4:
	sqlite3 TPC-H.db < preprocessing.sql; sqlite3 TPC-H.db < query4_opt.sql; sqlite3 TPC-H.db < clean.sql;

2:
	sqlite3 TPC-H.db < query2.sql

3:
	sqlite3 TPC-H.db < query3.sql

4:
	sqlite3 TPC-H.db < query4.sql

p:
	sqlite3 TPC-H.db < preprocessing.sql

c:
	sqlite3 TPC-H.db < clean.sql
