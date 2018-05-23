#include "lwfieldrendercommand.h"

double rendercommand_animtime(const LWFIELDRENDERCOMMAND* rcmd, double now) {
	return now - rcmd->animstarttime;
}
