#pragma once


#include<string>


class Client
{
public:

	Client();
	Client(std::string side);


	void set_side(std::string side);

	std::string get_side() const;

private:

	std::string side;
};

