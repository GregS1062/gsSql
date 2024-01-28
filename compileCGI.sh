g++ gsSqlCGI.cpp -g -o /var/www/html/gsSql.cgi -I /etc/-pedantic /usr/lib/libcgicc.a -I /usr/include/cgicc -Wall -Wextra -Werror -Wshadow -Wconversion -Wunreachable-code   2> error.txt -std=c++2a
[ -s error.txt ] && cat error.txt
chmod +x /var/www/html/gsSql.cgi
