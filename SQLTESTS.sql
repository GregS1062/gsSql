INSERT INTO table_name (column1, column2, column3, ...)
VALUES (value1, value2, value3, ...);

INSERT INTO table_name
VALUES (value1, value2, value3, ...);

INSERT INTO customer 
(deleted, custid, givenname, middleinitial, surname, phone, email, street1, street2, city, state, country, zipcode) VALUES (false, "000009196","Tony","A","Schiller","230-555-0191","tony20@adventure-works.com","5415 San Gabriel Dr.","NULL","Bothell","WA ","US","98011")

INSERT INTO customer 

VALUES (false, "000009196","Tony","A","Schiller","230-555-0191","tony20@adventure-works.com","5415 San Gabriel Dr.","NULL","Bothell","WA ","US
","98011")

UPDATE customer
SET surname = "Schmidt", City = "Frankfurt"
WHERE custid = "000000002";

