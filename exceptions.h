#include <exception>

class SiliconException: public exception {
public:
	SiliconException(std::string message) { msg += message; }

	virtual const char * what() const throw() {
		return msg.c_str();
	}

	virtual ~SiliconException() throw() {}
private:
	std::string msg = "";
};
