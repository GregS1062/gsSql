CREATE TABLE customers as "/var/www/html/bikeData/Customers.dat"
(
    deleted         bool,
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

CREATE INDEX customerid as "/var/www/html/bikeData/CustID.idx"
ON customers (custid)

CREATE INDEX customername as "/var/www/html/bikeData/CustName.idx"
ON customers (surname,givenname)

CREATE TABLE stores as "/var/www/html/bikeData/Stores.dat"
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

CREATE INDEX storeid as "/var/www/html/bikeData/StoreID.idx"
ON stores (custid)

CREATE INDEX storename as "/var/www/html/bikeData/StoreName.idx"
ON stores (name)

CREATE TABLE products as "/var/www/html/bikeData/Products.dat"
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

CREATE INDEX productid as "/var/www/html/bikeData/ProductID.idx"
ON products (productid)

CREATE TABLE orders as "/var/www/html/bikeData/Orders.dat"
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

CREATE INDEX orderid as "/var/www/html/bikeData/OrderID.idx"
ON orders (orderid)

CREATE INDEX customerorder as "/var/www/html/bikeData/CustomerOrder.idx"
ON orders (custid,orderid)

CREATE TABLE items as "/var/www/html/bikeData/Items.dat"
(
    deleted             bool,
	orderid         	char(11),
	productid       	char(11),
	quantity            int,
	price               double
	discount            double,
	total               double
)
CREATE INDEX orderproduct as "/var/www/html/bikeData/OrderProduct.idx"
ON items (orderid, productid)

CREATE INDEX productorder as "/var/www/html/bikeData/ProductOrder.idx"
ON items (productid,orderid)

