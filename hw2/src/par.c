/*********************/
/* par.c             */
/* for Par 3.20      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "errmsg.h"
#include "buffer.h"    /* Also includes <stddef.h>. */
#include "reformat.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <debug.h>
#include <stdbool.h>
#undef NULL
#define NULL ((void *) 0)

const char * const progname = "par";
const char * const version = "3.20";
static bool is_version = false; //Use to exit with SUCCESS rather than FAILURE
static void set_parseopt_error(char** argv);

static int digtoint(char c)
/* Returns the value represented by the digit c,   */
/* or -1 if c is not a digit. Does not use errmsg. */
{
  return c == '0' ? 0 :
         c == '1' ? 1 :
         c == '2' ? 2 :
         c == '3' ? 3 :
         c == '4' ? 4 :
         c == '5' ? 5 :
         c == '6' ? 6 :
         c == '7' ? 7 :
         c == '8' ? 8 :
         c == '9' ? 9 :
         -1;

  /* We can't simply return c - '0' because this is ANSI  */
  /* C code, so it has to work for any character set, not */
  /* just ones which put the digits together in order.    */
}


static int strtoudec(const char *s, int *pn)

/* Puts the decimal value of the string s into *pn, returning */
/* 1 on success. If s is empty, or contains non-digits,       */
/* or represents an integer greater than 9999, then *pn       */
/* is not changed and 0 is returned. Does not use errmsg.     */
{
  int n = 0;

  if (!*s) return 0;

  do {
    if (n >= 1000 || !isdigit(*s)) return 0;
    n = 10 * n + digtoint(*s);
  } while (*++s);

  *pn = n;

  return 1;
}


static void parseopt(
  int argc, char** argv, int *pwidth, int *pprefix,
  int *psuffix, int *phang, int *plast, int *pmin
)
/* Parses the single option in opt, setting *pwidth, *pprefix,     */
/* *psuffix, *phang, *plast, or *pmin as appropriate. Uses errmsg. */
{
  debug("Entered parse opt");
  int optchar, optIndex;
  //const char *saveopt = opt;
  //Last value in each nested struct is the short option that the long opt maps to
  //For min and last (long options), set the third field as the flag to change (ie, pmin or plast), and the last field to the value to change it to
  static int min , last;
  static struct option long_options[] = {
    {"width", required_argument, 0,  'W'},
    {"version", no_argument, 0,  'v'},
    {"prefix", required_argument, 0, 'P'},
    {"suffix", required_argument, 0,  'S'},
    {"min", no_argument, &min, 1},
    {"no-min", no_argument, &min,  0},
    {"hang", optional_argument, 0, 'H'},
    {"last", no_argument, &last , 1},
    {"no-last", no_argument, &last, 0}
  };
  //If ":" after a char, then the option has a required argument
  //If two ::, then optional: note that only the short options of last, hang, min should have optional args
  while ((optchar = getopt_long(argc, argv, "vw:W:p:P:s:S:m::h::H::l::",
  long_options, NULL)) != -1){
    debug("Opt char: %d\n", optchar);
    if (min != -1) *pmin = min;
    if (last != -1) *plast = last;
    switch(optchar){
      case 'v':{
        //For custom messages, open_memstream has to be used to print the message to a pointer, 
        //and then pass in the buffer to set_error before being freed
        char* ptr;
        size_t sizec;
        FILE* stream = open_memstream(&ptr, &sizec);
        fprintf(stream, "%s %s\n", progname, version);
        //After printing the specific error message to the buffer using fprintf, close the buffer 
        //The ptr passed into the buffer will then contain the error message to set to & print
        fclose(stream);
        set_error(ptr);
        free(ptr);
        is_version = true;
        return;
      }
      case 'w':
      case 'W':{
        if (strtoudec(optarg, pwidth) != 1) {set_parseopt_error(argv); return;}
        debug("Value of width: %d\n", *pwidth);
        break;
      }
      case 'p':
      case 'P':{
        if (strtoudec(optarg, pprefix) != 1) {set_parseopt_error(argv); return;}
        debug("Value of prefix: %d\n", *pprefix);
        break;
      }
      case 's':
      case 'S':{
        if (strtoudec(optarg, psuffix) != 1) {set_parseopt_error(argv); return;}
        debug("Value of suffix: %d\n", *psuffix);
        break;
      }
      //Optional arguments
      case 'h':
      case 'H':{
        if(optarg){
          if (strtoudec(optarg, phang) != 1) {set_parseopt_error(argv); return;}
        }
        debug("Value of hang: %d\n", *phang);
        break;
      }
      //last & min can both only be set to 0 or 1
      case 'l':{
        debug("Entered short last\n");
        if(optarg){
          if (strtoudec(optarg, plast) != 1 || *plast > 1) {set_parseopt_error(argv); return;}
        }
        debug("Value of last: %d\n", *plast);
        break;
      }
      case 'm':{
        if(optarg){
          if (strtoudec(optarg, pmin) != 1 || *pmin > 1) {set_parseopt_error(argv); return;}
        }
        debug("Value of min: %d\n", *pmin);
        break;
      }
      //Account for invalid options using case ?
      case '?':{
        set_parseopt_error(argv);
        return;
      }
    }
  }
  debug("Optind: %d\n", optind);
   //Check if all the command line arguments have been parsed as options:
  //If not, then parse each remaining arg using optind to configure width and prefix
  while (optind <= argc-1){
    int n, r;
    r = strtoudec(argv[optind], &n);
    if (r == 1){
      if (n>=9) *pwidth = n;
      else *pprefix = n;
      debug("Width: %d, Prefix: %d\n", *pwidth, *pprefix);
      optind++;
    }
    else{
      optind++;
      set_parseopt_error(argv);
      return;
    }
  }
  set_error('\0');
  return;
}
static void set_parseopt_error(char** argv){
  //For custom messages, open_memstream has to be used to print the message to a pointer, 
  //and then pass in the buffer to set_error before being freed
  char* ptr;
  size_t sizec;
  FILE* stream = open_memstream(&ptr, &sizec);
  fprintf(stream, "Bad option: %.149s\n", argv[optind-1]);
  //After printing the specific error message to the buffer using fprintf, close the buffer 
  //The ptr passed into the buffer will then contain the error message to set to & print
  fclose(stream);
  set_error(ptr);
  free(ptr);
}

static char **readlines(void)

/* Reads lines from stdin until EOF, or until a blank line is encountered, */
/* in which case the newline is pushed back onto the input stream. Returns */
/* a NULL-terminated array of pointers to individual lines, stripped of    */
/* their newline characters. Uses errmsg, and returns NULL on failure.     */
{
  struct buffer *cbuf = NULL, *pbuf = NULL;
  int c, blank;
  char ch, *ln, *nullline = NULL, nullchar = '\0', **lines = NULL;

  cbuf = newbuffer(sizeof (char));
  if (is_error()) goto rlcleanup;
  pbuf = newbuffer(sizeof (char *));
  if (is_error()) goto rlcleanup;

  for (blank = 1;  ; ) {
    c = getchar();
    if (c == EOF) break;
    if (c == '\n') {
      if (blank) {
        ungetc(c,stdin);
        break;
      }
      additem(cbuf, &nullchar);
      if (is_error()) goto rlcleanup;
      ln = copyitems(cbuf);
      if (is_error()) goto rlcleanup;
      additem(pbuf, &ln);
      if (is_error()) goto rlcleanup;
      clearbuffer(cbuf);
      blank = 1;
    }
    else {
      if (!isspace(c)) blank = 0;
      ch = c;
      additem(cbuf, &ch);
      if (is_error()) goto rlcleanup;
    }
  }

  if (!blank) {
    additem(cbuf, &nullchar);
    if (is_error()) goto rlcleanup;
    ln = copyitems(cbuf);
    if (is_error()) goto rlcleanup;
    additem(pbuf, &ln);
    if (is_error()) goto rlcleanup;
  }

  additem(pbuf, &nullline);
  if (is_error()) goto rlcleanup;
  lines = copyitems(pbuf);

rlcleanup:

  if (cbuf) freebuffer(cbuf);
  if (pbuf) {
    if (!lines){
      for (;;) {
        lines = nextitem(pbuf);
        if (!lines) break;
        free(*lines);
      }
    }
    freebuffer(pbuf); //MAKE SURE TO CLEAR PBUF BEFORE RETURNING
  }

  return lines;
}


static void setdefaults(
  const char * const *inlines, int *pwidth, int *pprefix,
  int *psuffix, int *phang, int *plast, int *pmin
)
/* If any of *pwidth, *pprefix, *psuffix, *phang, *plast, *pmin are     */
/* less than 0, sets them to default values based on inlines, according */
/* to "par.doc". Does not use errmsg because it always succeeds.        */
{
  int numlines;
  const char *start, *end, * const *line, *p1, *p2;

  if (*pwidth < 0) *pwidth = 72;
  if (*phang < 0) *phang = 0;
  if (*plast < 0) *plast = 0;
  if (*pmin < 0) *pmin = *plast;
  debug("Value of hang in setdefault: %d\n", *phang);

  for (line = inlines;  *line;  ++line);
  numlines = line - inlines;

  if (*pprefix < 0){
    if (numlines <= *phang + 1)
      *pprefix = 0;
    else {
      start = inlines[*phang];
      for (end = start;  *end;  ++end);
      for (line = inlines + *phang + 1;  *line;  ++line) {
        for (p1 = start, p2 = *line;  p1 < end && *p1 == *p2;  ++p1, ++p2);
        end = p1;
      }
      *pprefix = end - start;
    }
  }

  if (*psuffix < 0){
    if (numlines <= 1)
      *psuffix = 0;
    else {
      start = *inlines;
      for (end = start;  *end;  ++end);
      for (line = inlines + 1;  *line;  ++line) {
        for (p2 = *line;  *p2;  ++p2); //MAKE SURE p2 IS CORRECTLY ADVANCED 
        for (p1 = end;
             p1 > start && p2 > *line && p1[-1] == p2[-1];
             --p1, --p2);
        start = p1;
      }
      while (end - start >= 2 && isspace(*start) && isspace(start[1])) ++start;
      *psuffix = end - start;
    }
  }
}


static void freelines(char **lines)
/* Frees the strings pointed to in the NULL-terminated array lines, then */
/* frees the array. Does not use errmsg because it always succeeds.      */
{
  char **line;
  //MAKE SURE THE LOOP ADVANCES line correctly
  for (line = lines;  *line;  ++line)
    free(*line);

  free(lines);
}


int original_main(int argc, char **argv)
{
  int width, widthbak = -1, prefix, prefixbak = -1, suffix, suffixbak = -1,
      hang, hangbak = -1, last, lastbak = -1, min, minbak = -1, c;
  char *parinit, *picopy = NULL, *opt, **inlines = NULL, **outlines = NULL,
       **line=NULL;
  const char * const whitechars = " \f\n\r\t\v";
  //PARINIT performs the same function as cmd input-- ie PARINIT= "bin/par -w 20"
  parinit = getenv("PARINIT");
  if (parinit) {
    picopy = malloc((strlen(parinit) + 1) * sizeof (char));
    if (!picopy) {
      set_error((char*)outofmem);
      goto parcleanup;
    }
    strcpy(picopy,parinit);
    opt = strtok(picopy,whitechars);
    int argc2 = 0;
    char** argv2 = malloc(0);
    //Loop through each opt, updating arc2 and putting each opt into argv2
    while (opt) {
      argc2++;
      argv2 = realloc(argv2, (sizeof(char*) * argc2));
      argv2[argc2-1] = opt;
      debug("Par element: %s\n", argv2[argc2-1]);
      if (is_error()){
        free(argv2);
        goto parcleanup;
      }
      opt = strtok(NULL,whitechars);
    }
    //Once done looping, call parseopt w argc2 and argv2
    parseopt(argc2, argv2, &widthbak, &prefixbak,
            &suffixbak, &hangbak, &lastbak, &minbak);
    free(argv2);
    if (is_error()) goto parcleanup;
    free(picopy);
    picopy = NULL;
    optind = 1;
  }
  parseopt(argc, argv, &widthbak, &prefixbak,
            &suffixbak, &hangbak, &lastbak, &minbak);
  if (is_error()) goto parcleanup;


  for (;;) {
    for (;;) {
      c = getchar();
      if (c != '\n') break;
      putchar(c);
    }
    ungetc(c,stdin);

    inlines = readlines();
    if (is_error()) goto parcleanup;
    if (!*inlines) {
      free(inlines);
      inlines = NULL;
      //MAKE SURE THE LOOP TERMINATES
      if (c != EOF) continue;
      else break;
    }

    width = widthbak;  prefix = prefixbak;  suffix = suffixbak;
    hang = hangbak;  last = lastbak;  min = minbak;
    setdefaults((const char * const *) inlines,
                &width, &prefix, &suffix, &hang, &last, &min);

    outlines = reformat((const char * const *) inlines,
                        width, prefix, suffix, hang, last, min);
    if (is_error()) goto parcleanup;

    freelines(inlines);
    inlines = NULL;

    for (line = outlines;  *line;  ++line)
      puts(*line);

    freelines(outlines);
    outlines = NULL;
  }

parcleanup:

  if (picopy) free(picopy);
  if (inlines) freelines(inlines);
  if (outlines) freelines(outlines);

  if (is_error()) {
    report_error(stderr);
    clear_error();
    exit(is_version ? EXIT_SUCCESS : EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}