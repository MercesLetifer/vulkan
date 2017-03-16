#include "vulkanapp.h"
#include <iostream>
#include <conio.h>

int main()
{
	VulkanApp app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		_getch();
		return -1;
	}

	_getch();
	return 0;
}
