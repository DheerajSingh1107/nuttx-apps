#ifndef __MYAPP_H
#define __MYAPP_H

#ifdef __cplusplus
extern "C" {
#endif

int button_daemon(int argc, FAR char *argv[]);
int led_daemon(int argc, FAR char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* __MYAPP_H */