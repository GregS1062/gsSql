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
    zipcode         char(11)
)  

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
    zipcode         char(11)
)

CREATE TABLE products as "/var/www/html/bikeData/Products.dat"
(
    deleted             bool,
	product_number      char(11),
	description         char(51),
	color               char(16),
	size                char(6),
	safetystocklevel    int,
	reorderpoint        int,
	standardcost        double,
	listprice           double,
	sellstartdate       date,
	sellenddate         date

)

CREATE TABLE orders as "/var/www/html/bikeData/Orders.dat"
(
    deleted             bool,
	status              int,
	ordernumber         char(11),
	custid         		char(11),
	orderdate           date,
	duedate             date,
	shipdate            date,
	tax                 double,
	freight             double,
	totaldue            double
)

CREATE TABLE items as "/var/www/html/bikeData/Items.dat"
(
    deleted             bool,
	ordernumber         char(11),
	productnumber       char(11),
	quantity            int,
	price               double
	discount            double,
	total               double

)
