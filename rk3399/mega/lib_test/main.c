// main.c
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>

int* LOAD_LIB(char filename[])
{
    void *handle;

    handle = dlopen(filename, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        return handle;
    }

    dlerror();    /* 清除任何现有的错误 */
    return handle;
}

int *LOAD_SYM(void *handle, char symbol[])
{
    int *result;
    *(void **) (&result) = dlsym(handle, symbol);
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        fprintf(stderr, "%s\n", dlsym_error);
        dlclose(handle);
        return NULL;
    }
    return result;
}

void *server_thread(void *arg) {
    printf("This is thread_server\n");
    return NULL;
}

int main() {
    void *handle_server, *handle_client;
    void (*server)();
    void (*client)();

    handle_server = LOAD_LIB("./libserver.so");
    handle_client = LOAD_LIB("./libclient.so");

    dlerror();

    server = LOAD_SYM(handle_server, "server");
    client = LOAD_SYM(handle_client, "client");

    pthread_t thread_server, thread_client;
    pthread_create(&thread_server, NULL, (void *(*)(void *))server, NULL);
    pthread_create(&thread_client, NULL, (void *(*)(void *))client, NULL);

    pthread_join(thread_server, NULL);
    pthread_join(thread_client, NULL);

    dlclose(handle_server);
    dlclose(handle_client);
    return 0;
}
