#ifndef __MYAPP_H
#define __MYAPP_H

#define MQ_NAME "/button_mq"
#define MQ_MAX_MSG 10
#define MQ_MSG_SIZE sizeof(int)

#ifdef __cplusplus
extern "C" {
#endif

int button_daemon(int argc, FAR char *argv[]);
int led_daemon(int argc, FAR char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* __MYAPP_H */