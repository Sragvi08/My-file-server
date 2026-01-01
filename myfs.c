#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>

/*
 Usage:
   myfs 0 <port>           ## server mode
   myfs 1 <host> <port>    ## client mode
*/

#define MAXT       2
#define BUFSIZE    15000
#define TIMELIMIT  5000
#define LISTENQ    8
#define MEMMASTER  20000

struct calc_data {
  double a;
  double b;
  char   op;
};

/* forward declarations */
int Server(char **);
int Client(char **);
void *Worker(void *);
double doCalc(int, void *);

int main(int c, char **v)
{
  int r = 0;
  switch (atoi(v[1])) {
    case 0: r = Server(v); break;
    case 1: r = Client(v); break;
    default: break;
  }
  exit(r);
}

/* ------------------- CALCULATION ------------------- */

double doCalc(int num, void *data)
{
  (void)num;
  struct calc_data *d = (struct calc_data *)data;
  double result = 0.0;

  switch ((unsigned char)d->op) {
    case '+': result = d->a + d->b; break;
    case '-': result = d->a - d->b; break;
    case '*': result = d->a * d->b; break;
    case '/':
      if (d->b == 0.0) result = 0.0;
      else result = d->a / d->b;
      break;
    case '^':
      result = pow(d->a, d->b);
      break;
    default:
      result = 0.0;
      break;
  }

  /* Final safety: any NaN / Inf => error */
  if (!isfinite(result)) result = 0.0;
  return result;
}

/* ------------------- WORKER THREAD ------------------- */

void *Worker(void *p)
{
  int sd = (int)(intptr_t)p;
  pthread_t me = pthread_self();
  printf("I am thread %lu\n", me);

  char *buffer_block = malloc(BUFSIZE);
  if (!buffer_block) {
    close(sd);
    pthread_exit(NULL);
  }

  int READ_BUF_SIZE = BUFSIZE - 1000;
  int WRITE_BUF_SIZE = BUFSIZE - READ_BUF_SIZE;

  char *read_buffer  = buffer_block;
  char *write_buffer = buffer_block + READ_BUF_SIZE;

#define MAXNUMS 64
#define MAXOPS  64
  double nums[MAXNUMS];
  char   ops[MAXOPS];

  size_t remaining_len = 0;
  size_t write_len = 0;
  double local_total = 0.0;

  struct calc_data d;
  char result_string[1024];

  for (;;) {
    ssize_t n = read(sd, read_buffer + remaining_len,
                     READ_BUF_SIZE - remaining_len);
    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    }

    size_t total_data = remaining_len + (size_t)n;
    bool client_finished = (n == 0);

    if (client_finished && total_data > 0 &&
        read_buffer[total_data - 1] != '\n') {
      read_buffer[total_data++] = '\n';
    }

    if (total_data == 0) break;

    char *line_start = read_buffer;
    size_t processed_len = 0;

    for (size_t i = 0; i < total_data; i++) {
      if (read_buffer[i] == '\n') {
        char old = read_buffer[i];
        read_buffer[i] = '\0';

        int num_count = 0, op_count = 0;
        bool garbage_val = false;
        char *p = line_start;

        while (*p) {
          while (*p == ' ') p++;

          int chars = 0;
          if (sscanf(p, "%lf%n", &nums[num_count], &chars) == 1) {
            /* Reject negative numbers */
            if (nums[num_count] < 0.0) {
              garbage_val = true;
              break;
            }
            p += chars;
            num_count++;
            if (num_count >= MAXNUMS) { garbage_val = true; break; }
          } else {
            garbage_val = true;
            break;
          }

          while (*p == ' ') p++;
          if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '^') {
            ops[op_count++] = *p++;
            if (op_count >= MAXOPS) { garbage_val = true; break; }
          } else break;
        }

        while (*p == ' ') p++;
        if (*p != '\0') garbage_val = true;
        if (op_count != num_count - 1) garbage_val = true;

        double final_result = 0.0;

        if (!garbage_val) {

          /* Exponentiation: right to left */
          for (int j = op_count - 1; j >= 0; j--) {
            if (ops[j] == '^') {
              d.a = nums[j];
              d.b = nums[j + 1];
              d.op = '^';
              nums[j] = doCalc(2, &d);

              for (int k = j + 1; k < num_count - 1; k++)
                nums[k] = nums[k + 1];
              for (int k = j; k < op_count - 1; k++)
                ops[k] = ops[k + 1];

              num_count--;
              op_count--;
            }
          }

          /* Multiply / Divide */
          for (int j = 0; j < op_count; j++) {
            if (ops[j] == '*' || ops[j] == '/') {
              d.a = nums[j];
              d.b = nums[j + 1];
              d.op = ops[j];
              nums[j] = doCalc(2, &d);

              for (int k = j + 1; k < num_count - 1; k++)
                nums[k] = nums[k + 1];
              for (int k = j; k < op_count - 1; k++)
                ops[k] = ops[k + 1];

              num_count--;
              op_count--;
              j--;
            }
          }

          /* Add / Sub */
          double acc = nums[0];
          for (int j = 0; j < op_count; j++) {
            d.a = acc;
            d.b = nums[j + 1];
            d.op = ops[j];
            acc = doCalc(2, &d);
          }

          final_result = acc;
        }

        local_total += final_result;

        int fl = snprintf(result_string, sizeof(result_string),
                          "%s %.3f\n", line_start, final_result);

        if (write_len + fl > WRITE_BUF_SIZE) {
          write(sd, write_buffer, write_len);
          write_len = 0;
        }

        memcpy(write_buffer + write_len, result_string, fl);
        write_len += fl;

        read_buffer[i] = old;
        line_start = read_buffer + i + 1;
        processed_len = i + 1;
      }
    }

    remaining_len = total_data - processed_len;
    memmove(read_buffer, line_start, remaining_len);
    if (client_finished) break;
  }

  if (write_len > 0)
    write(sd, write_buffer, write_len);

  double *ret = (double *)(buffer_block + BUFSIZE - sizeof(double));
  *ret = local_total;

  close(sd);
  pthread_exit(ret);
}

/* ------------------- SERVER ------------------- */

int Server(char **v)
{
  struct sockaddr_in sa, ca;
  socklen_t slen;
  int sd;

  struct timeval start, now;
  gettimeofday(&start, NULL);

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(atoi(v[2]));

  sd = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sd, F_SETFL, O_NONBLOCK);
  bind(sd, (struct sockaddr *)&sa, sizeof(sa));
  listen(sd, LISTENQ);

  void *master_block = malloc(MEMMASTER);
  pthread_t *tids = master_block;
  int tid_count = 0;
  double grand_total = 0.0;
  int total_clients = 0;

  for (;;) {
    for (int i = 0; i < tid_count; i++) {
      void *ret = NULL;
      if (pthread_tryjoin_np(tids[i], &ret) == 0) {
        grand_total += *(double *)ret;
        free((char *)ret - (BUFSIZE - sizeof(double)));
        tids[i] = tids[--tid_count];
        i--;
      }
    }

    gettimeofday(&now, NULL);
    long elapsed =
      (now.tv_sec - start.tv_sec) * 1000 +
      (now.tv_usec - start.tv_usec) / 1000;
    if (elapsed >= TIMELIMIT) break;

    slen = sizeof(ca);
    int cfd = accept(sd, (struct sockaddr *)&ca, &slen);
    if (cfd < 0) continue;

    if (tid_count >= MAXT) {
      close(cfd);
      continue;
    }

    pthread_create(&tids[tid_count++], NULL,
                   Worker, (void *)(intptr_t)cfd);
    total_clients++;
  }

  for (int i = 0; i < tid_count; i++) {
    void *ret;
    pthread_join(tids[i], &ret);
    grand_total += *(double *)ret;
    free((char *)ret - (BUFSIZE - sizeof(double)));
  }

  printf("total %d clients total result %.3f\n",
         total_clients, grand_total);

  free(master_block);
  close(sd);
  return 0;
}

/* ------------------- CLIENT ------------------- */

int Client(char **v)
{
  struct sockaddr_in sa;
  int sd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  inet_aton(v[2], &sa.sin_addr);
  sa.sin_port = htons(atoi(v[3]));

  connect(sd, (struct sockaddr *)&sa, sizeof(sa));

  char *buf = malloc(BUFSIZE);
  ssize_t n;

  while ((n = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
    write(sd, buf, n);

  shutdown(sd, SHUT_WR);

  while ((n = read(sd, buf, BUFSIZE)) > 0)
    write(STDOUT_FILENO, buf, n);

  free(buf);
  close(sd);
  return 0;
}
