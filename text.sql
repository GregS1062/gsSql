
 databases 		databases:
 database 			database:	 value=bike
 tables 			tables:
 table 				table:	 value=customer
 location 				location:	 value=Customers.dat
 columns 				columns:
 column 					column:	 value=deleted
 type 					type:
 char 						char:	 value=1,
 column 					column:	 value=custid
 type 					type:
 char 						char:	 value=11,
 column 					column:	 value=givenname
 type 					type:
 char 						char:	 value=21,
 column 					column:	 value=middleinitial
 type 					type:
 char 						char:	 value=2,
 column 					column:	 value=surname
 type 					type:
 char 						char:	 value=21,
 column 					column:	 value=phone
 type 					type:
 char 						char:	 value=21,
 column 					column:	 value=email
 type 					type:
 char 						char:	 value=41,
 column 					column:	 value=street1
 type 					type:
 char 						char:	 value=31,
 column 					column:	 value=street2
 type 					type:
 char 						char:	 value=31,
 column 					column:	 value=city
 type 					type:
 char 						char:	 value=21,
 column 					column:	 value=state
 type 					type:
 char 						char:	 value=4,
 column 					column:	 value=country
 type 					type:
 char 						char:	 value=4,
 column 					column:	 value=zipcode
 type 					type:
 char 						char:	 value=11,	 value=indexes,
 value = null		
 index 				index:	 value=customerNumber
 location 				location:	 value=customerNumber.idx
 columns 				columns:
 column 					column:	 value=custid
 type 					type:
 char 						char:	 value=11,	 value=,
 index 				index:	 value=customerName
 location 				location:	 value=customerName.idx
 columns 				columns:
 column 					column:	 value=surname
 type 					type:
 char 						char:	 value=21,
 column 					column:	 value=givenname
 type 					type:
 char 						char:	 value=21,	 value=,	 value=,
 table 			table:	 value=store
 location 			location:	 value=store.dat
 columns 			columns:
 column 				column:	 value=store_id
 type 				type:
 char 					char:	 value=11,
 column 				column:	 value=store_name
 type 				type:
 char 					char:	 value=30,	 value=
 key = table
 key = location
 key = columns
 table = customer