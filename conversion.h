#include <time.h>

class Addresses
{
public:
	char	Street1[31];
	char	Street2[31];
	char	City[21];
	char	State[4];
	char	Country[4];
	char	ZipCode[11];
};
class Persons
{
public:
	char		GivenName[21];
	char		MiddleInitial[2];
	char		SurName[21];
	char		Phone[21];
	char		EMAIL[41];
};
class Customers
{
public:
	bool		Deleted;
	char		Customer_ID[11];
	Persons		Person;
	Addresses	Address;
};
class Stores
{
public:
	bool		Deleted;
	char		Customer_ID[11];
	char		Name[31];
	char		Phone[21];
	char		EMAIL[41];
	Addresses	Address;
};
class Products
{
public:
	bool	Deleted;
	char	Product_Number[11];
	char	Description[51];
	char	Color[16];
	char	Size[6];
	int		SafetyStockLevel;
	int		ReorderPoint;
	double	StandardCost;
	double	ListPrice;
	struct tm SellStartDate;
	struct tm SellEndDate;
};

class Orders
{
public:
	bool	Deleted;
	int		Status;
	char	Order_Number[11];
	char	Customer_ID[11];
	struct tm OrderDate;
	struct tm DueDate;
	struct tm ShipDate;
	double	Tax;
	double	Freight;
	double	TotalDue;
};
class Items
{
public:
	bool	Deleted;
	char	Order_Number[11];
	char	Product_Number[11];
	int		Quantity;
	double	Price;
	double	Discount;
	double	Total;
};
