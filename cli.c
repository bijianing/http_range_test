#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#if 1
#define DbgPrint(f, x...)      printf("Dbg: %s():" f, __func__, ##x)
#else
#define DbgPrint(f, x...)
#endif

#define FN "dl_out"

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int dl(const char *fn, char range_header)
{
    CURL *curl;
    struct curl_slist *list = NULL;
    FILE *fp;
    int ret = 0;
    char path[128];

    sprintf(path, "http://localhost:8888/%s", fn);

    // curlのセットアップ
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, path);

    list = curl_slist_append(list, range_header);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    if (unlink(FN) != 0) {
        ret = 1;
        goto out;
    }

    if ((fp = fopen(FN, "wb")) == NULL) {
        ret = 2;
        goto out;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    // 実行
    if (curl_easy_perform(curl) != 0) {
        ret = 3;
        goto out;
    }

    ret = 0;

out:
    // 後始末
    curl_slist_free_all(list); /* free the list again */
    curl_easy_cleanup(curl);
    fclose(fp);

    return ret;
}

#define BUFSZ   (2*1024*1024)
int compare_out(const char *fn1, const char *fn2)
{
    int ret;
    int fd1, fd2;
    char buf1[BUFSZ], buf2[BUFSZ];

    if ((fd1 = open(fn1, O_RDONLY)) < 0) {

    }
}

void usage(const char *cmd)
{
    printf("Usage: %s <file> <start> <end> \n", cmd);
}

int main(int argc, const char *argv[])
{
    long start, end;
    char rheader[128];

    if (argc != 4) {
        usage(argv[0]);
        return -1;
    }
    if ((start = atol(argv[2])) < 0) {
        fprintf(stderr, "start is minus\n");
        return -2;
    }
    if ((end = atol(argv[3])) <= 0) {
        fprintf(stderr, "end is too small\n");
        return -3;
    }
    sprintf(rheader, "Range: bytes=%l-%l", start, end);

    return EXIT_SUCCESS;
}

