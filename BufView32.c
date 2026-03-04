void writeIntToBuff(int i, int index, char *buff) {
	buff[index] = (char)(i>>24);
	buff[index+1] = (char)(i>>16);
	buff[index+2] = (char)(i>>8);
	buff[index+3] = (char)(i);
}

int readIntFromBuff(const int index, const char *buff) {
	int i = 0;

	i |= ((int)(buff[index])<<24);
	i |= ((int)(buff[index+1])<<16);
	i |= ((int)(buff[index+2])<<8);
	i |= ((int)buff[index+3]);

	return i;
}
