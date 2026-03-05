void writeIntToBuff(int i, int index, unsigned char *buff) {
	buff[index] = (unsigned char)(i>>24);
	buff[index+1] = (unsigned char)(i>>16);
	buff[index+2] = (unsigned char)(i>>8);
	buff[index+3] = (unsigned char)(i);
}

int readIntFromBuff(const int index, const unsigned char *buff) {
	int i = 0;

	i |= ((int)(buff[index])<<24);
	i |= ((int)(buff[index+1])<<16);
	i |= ((int)(buff[index+2])<<8);
	i |= ((int)buff[index+3]);

	return i;
}
