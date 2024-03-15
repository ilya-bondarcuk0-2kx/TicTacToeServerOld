#include "Client.h"

Client::Client()
	:side("")
{
}

Client::Client(std::string side)
	:side(side)
{
}

void Client::set_side(std::string side)
{
	this->side = side;
}

std::string Client::get_side() const
{
	return side;
}
