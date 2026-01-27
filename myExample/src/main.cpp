#include "vulkanManager.h"

int main() {
	if (!CVulkanManager::getInstance().valid())
		return 1;

	return 0;
}
