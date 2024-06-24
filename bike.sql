CREATE TABLE customers AS "/home/greg/projects/test/testData/Customers.dat" 
( 
    deleted			bool,
    custid          char(11),
    givenname       char(21),
    middleinitial   char(2),
    surname         char(21),
    phone           char(21),
    email           char(41),
    street1         char(31),
    street2         char(31),
    city            char(21),
    state           char(4),
    country         char(4),
    zipcode         char(11),
	PRIMARY KEY (custid)
)  

CREATE INDEX customerid AS "/home/greg/projects/test/testIndex/custid.idx"
ON customers (custid)


CREATE TABLE stores AS "/home/greg/projects/test/testData/Stores.dat"
(
    deleted         bool,
    custid          char(11),
	name            char(31),
    phone           char(21),
    email           char(41),
	street1         char(31),
    street2         char(31),
    city            char(21),
    state           char(4),
    country         char(4),
    zipcode         char(11),
	PRIMARY KEY (custid)
)

CREATE INDEX storeid AS "/home/greg/projects/test/testIndex/storeid.idx"
ON stores (custid)

CREATE INDEX storename AS "/home/greg/projects/test/testIndex/storename.idx"
ON stores (name)

CREATE TABLE products AS "/home/greg/projects/test/testData/Products.dat"
(
    deleted             bool,
	productid       	char(11),
	description         char(51),
	color               char(16),
	size                char(6),
	safetystocklevel    int,
	reorderpoint        int,
	standardcost        double,
	listprice           double,
	sellstartdate       date,
	sellenddate         date,
	PRIMARY KEY (productid)
)

CREATE INDEX productid AS "/home/greg/projects/test/testIndex/productid.idx"
ON products (productid)

CREATE TABLE orders AS "/home/greg/projects/test/testData/Orders.dat"
(
    deleted             bool,
	status              int,
	orderid         	char(11),
	custid         		char(11),
	orderdate           date,
	duedate             date,
	shipdate            date,
	tax                 double,
	freight             double,
	totaldue            double,
	PRIMARY KEY (orderid)
)

CREATE INDEX orderid AS "/home/greg/projects/test/testIndex/orderid.idx"
ON orders (orderid)


CREATE TABLE items AS "/home/greg/projects/test/testData/Items.dat"
(
    deleted             bool,
	orderid         	char(11),
	productid       	char(11),
	quantity            int,
	price               double,
	discount            double,
	total               double,
	PRIMARY KEY (orderid)
)

CREATE INDEX orderid AS "/home/greg/projects/test/testIndex/itemOrder.idx"
ON items (orderid)

CREATE TABLE functionTable AS "systable"
(
	count			int,
	max				double,
	min				double,
	sum				double,
	avg				double
)

