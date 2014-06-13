#if defined (_WIN32) || defined (_WIN64)
typedef HANDLE	sema_t;
#else
typedef struct semaphore_t {
	char name[64];
	sem_t *sema;
} *sema_t;
#endif


sema_t sema_create(int key, int cnt = 1);
int sema_try_enter(sema_t sema, unsigned long millsec);
int sema_enter(sema_t sema);
int sema_leave(sema_t sema);
void sema_del(sema_t sema);

