%:%.cpp
	g++ $< -o $@ -g -I ./inc/ -lpthread -lmysqlclient
