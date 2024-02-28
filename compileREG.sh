g++ regTest.cpp -g -o /home/greg/projects/regTest/regTest -I /etc/-pedantic -I /usr/include/cgicc -Wall -Wextra -Werror -Wshadow -Wconversion -Wunreachable-code -Wno-pointer-arith  2> error.txt -std=c++2a

[ -s error.txt ] && cat error.txt 
