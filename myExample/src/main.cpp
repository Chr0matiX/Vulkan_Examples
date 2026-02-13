#include "VkContext.h"
#include "vkContext.h"

int main() {
	if (!VkContext::getInstance().valid())
		return 1;

	return 0;
}
