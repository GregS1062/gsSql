INSERT INTO table_name (column1, column2, column3, ...)
VALUES (value1, value2, value3, ...);

INSERT INTO table_name
VALUES (value1, value2, value3, ...);

INSERT INTO customer 
(deleted, custid, givenname, middleinitial, surname, phone, email, street1, street2, city, state, country, zipcode) VALUES (false, "000009196","Tony","A","Schiller","230-555-0191","tony20@adventure-works.com","5415 San Gabriel Dr.","NULL","Bothell","WA ","US","98011")

INSERT INTO customer 

VALUES (0, "000009196","Tony","A","Schiller","230-555-0191","tony20@adventure-works.com","5415 San Gabriel Dr.","NULL","Bothell","WA ","US
","98011")

UPDATE customer
SET surname = "Schmidt", City = "Frankfurt"
WHERE custid = "000000002";

update customer set street1 = "324 4th Street SW" where custid = "000000011"

INSERT INTO orders 
(deleted, status, ordernumber,custid,orderdate,duedate,shipdate,tax,freight,totaldue) Values
(0,1,"S0003","000002","03/14/2024","03/15/2024","0/0/0",14.34,10.00,2001)

INSERT INTO Products (deleted,product_number,description,color,size,safetystocklevel,reorderpoint,standardcost,listprice,sellstartdate,sellenddate) Values (0,"MXD-142","A big honken MXD","Red","large", 6,2,149.00,250.00,"03/14/2024","03/14/2025")

Insert into items 
(deleted,ordernumber,productnumber,quantity,price,discount,total)
Values (0,"S0004","MDX145",1,140.99,0,140.99)

