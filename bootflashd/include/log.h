#ifndef _LOG_H_
#define _LOG_H_

#define log(format, ...) do { fprintf(stderr, format "\n", ##__VA_ARGS__); } while(0)
#define die(format, ...) do { log(format, ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)

#endif /* _LOG_H_ */
