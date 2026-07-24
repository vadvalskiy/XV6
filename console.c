// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}


void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

static int
cga_getpos(void)
{
  int pos;
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);
  return pos;
}

static void
cga_setpos(int pos)
{
  if(pos < 0) pos = 0;
  if(pos > 25*80) pos = 25*80;
  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
}

static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
    cgaputc(c);
    return;
  }

  if (c == LEFT_ARROW) {
    uartputc('\b');
    {
      int pos = cga_getpos();
      if (pos > 0) cga_setpos(pos - 1);
    }
    return;
  }

  if (c == RIGHT_ARROW) {
    {
      int pos = cga_getpos();
      cga_setpos(pos + 1);
    }
    return;
  }

  uartputc(c);
  cgaputc(c);
}

#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

// Lab 1 line editor state.  curpos is a logical position in the
// current input line, not an absolute CGA cursor location.
static int curpos = 0;
static int input_start_pos = -1;
static int input_drawn_len = 0;
static int serial_cursor_pos = 0;

static char selected_buffer[INPUT_BUF];
static int selection_active = 0;       // first Ctrl+S has been pressed
static int selection_start_index = -1; // active anchor or final range start
static int selection_end_index = -1;   // final range end; -1 means no final selection

// A parallel array for the current line.  Each inserted character receives a
// monotonically increasing stamp.  Ctrl+Z deletes the still-existing character
// with the greatest stamp, which matches "last character inserted in time".
static int input_stamp[INPUT_BUF];
static int next_input_stamp = 1;

#define TAB_KEY 0x09
#define MAX_MATCHES 50
#define MAX_FILENAME_LEN DIRSIZ
#define CGA_INPUT_LIMIT (24*80)

static int
line_len(void)
{
  return input.e - input.w;
}

static char
line_get(int pos)
{
  return input.buf[(input.w + pos) % INPUT_BUF];
}

static void
line_set(int pos, char c)
{
  input.buf[(input.w + pos) % INPUT_BUF] = c;
}

static void
clear_selection_state(void)
{
  selection_active = 0;
  selection_start_index = -1;
  selection_end_index = -1;
}

static int
selection_final(void)
{
  return selection_end_index >= 0;
}

static int
clamp_pos(int p)
{
  int len = line_len();
  if(p < 0)
    return 0;
  if(p > len)
    return len;
  return p;
}

static void
ensure_input_start(void)
{
  if(input_start_pos < 0)
    input_start_pos = cga_getpos();
}

static void
move_cursor_serial(void)
{
  while(serial_cursor_pos > curpos){
    uartputc('\b');
    serial_cursor_pos--;
  }
  while(serial_cursor_pos < curpos && serial_cursor_pos < line_len()){
    // Reprinting the existing character advances terminals that do not
    // implement ANSI cursor-right sequences.
    uartputc(line_get(serial_cursor_pos));
    serial_cursor_pos++;
  }
}

static void
move_cursor_cga(void)
{
  move_cursor_serial();
  if(input_start_pos >= 0){
    int pos = input_start_pos + curpos;
    if(pos >= CGA_INPUT_LIMIT)
      pos = CGA_INPUT_LIMIT - 1;
    cga_setpos(pos);
  }
}

static void
redraw_serial_input(int clear_to)
{
  int i;

  while(serial_cursor_pos > 0){
    uartputc('\b');
    serial_cursor_pos--;
  }
  for(i = 0; i < line_len(); i++){
    uartputc(line_get(i));
    serial_cursor_pos++;
  }
  for(i = line_len(); i < clear_to; i++){
    uartputc(' ');
    serial_cursor_pos++;
  }
  move_cursor_serial();
}

static void
redraw_input(void)
{
  int i;
  int len = line_len();
  int clear_to = input_drawn_len;

  ensure_input_start();
  if(clear_to < len)
    clear_to = len;

  // If editing would cross xv6's last usable VGA cell, start a fresh line.
  // This avoids direct framebuffer writes past the scroll boundary.
  if(input_start_pos + clear_to >= CGA_INPUT_LIMIT){
    cgaputc('\n');
    input_start_pos = cga_getpos();
    input_drawn_len = 0;
    clear_to = len;
  }

  redraw_serial_input(clear_to);

  for(i = 0; i <= clear_to; i++){
    int pos = input_start_pos + i;
    if(pos >= 0 && pos < CGA_INPUT_LIMIT)
      crt[pos] = ' ' | 0x0700;
  }

  for(i = 0; i < len; i++){
    int pos = input_start_pos + i;
    ushort attr = 0x0700;
    if(selection_final() && i >= selection_start_index && i < selection_end_index)
      attr = 0x7000;
    if(pos >= 0 && pos < CGA_INPUT_LIMIT)
      crt[pos] = (line_get(i) & 0xff) | attr;
  }

  input_drawn_len = len;
  curpos = clamp_pos(curpos);
  move_cursor_cga();
}

static void
reset_line_editor(void)
{
  int i;
  curpos = 0;
  input_start_pos = -1;
  input_drawn_len = 0;
  serial_cursor_pos = 0;
  clear_selection_state();
  for(i = 0; i < INPUT_BUF; i++)
    input_stamp[i] = 0;
  next_input_stamp = 1;
}

static void
adjust_active_for_insert(int pos, int n)
{
  if(selection_active && selection_end_index < 0 && pos <= selection_start_index)
    selection_start_index += n;
}

static void
adjust_active_for_delete(int pos, int n)
{
  int end = pos + n;

  if(!(selection_active && selection_end_index < 0))
    return;

  if(end <= selection_start_index)
    selection_start_index -= n;
  else if(pos < selection_start_index)
    selection_start_index = pos;

  selection_start_index = clamp_pos(selection_start_index);
}

static int
can_insert(int n)
{
  if(n <= 0)
    return 0;
  return (int)(input.e - input.r) + n < INPUT_BUF;
}

static int
insert_bytes_at(int pos, char *src, int n)
{
  int i;
  int len = line_len();

  if(n <= 0)
    return 1;
  if(pos < 0 || pos > len)
    return 0;
  if(!can_insert(n))
    return 0;

  ensure_input_start();
  adjust_active_for_insert(pos, n);

  for(i = len - 1; i >= pos; i--){
    line_set(i + n, line_get(i));
    input_stamp[i + n] = input_stamp[i];
  }
  for(i = 0; i < n; i++){
    line_set(pos + i, src[i]);
    input_stamp[pos + i] = next_input_stamp++;
    if(next_input_stamp <= 0)
      next_input_stamp = 1;
  }

  input.e += n;
  curpos = pos + n;
  return 1;
}

static int
delete_range_at(int pos, int n, int newcur)
{
  int i;
  int len = line_len();

  if(n <= 0)
    return 1;
  if(pos < 0 || pos >= len)
    return 0;
  if(pos + n > len)
    n = len - pos;

  adjust_active_for_delete(pos, n);

  for(i = pos; i + n < len; i++){
    line_set(i, line_get(i + n));
    input_stamp[i] = input_stamp[i + n];
  }
  for(; i < len; i++)
    input_stamp[i] = 0;

  input.e -= n;
  curpos = clamp_pos(newcur);
  return 1;
}

static int
delete_final_selection(void)
{
  int start, end;

  if(!selection_final())
    return 0;
  start = selection_start_index;
  end = selection_end_index;
  clear_selection_state();
  return delete_range_at(start, end - start, start);
}

static void
replace_final_selection(char *src, int n)
{
  int start;

  if(!selection_final())
    return;
  start = selection_start_index;
  delete_final_selection();
  if(n > 0)
    insert_bytes_at(start, src, n);
}

static void
copy_final_selection(void)
{
  int i, j = 0;

  if(!selection_final())
    return;
  for(i = selection_start_index; i < selection_end_index && j < INPUT_BUF - 1; i++)
    selected_buffer[j++] = line_get(i);
  selected_buffer[j] = 0;
}

static int
delete_latest_inserted_char(void)
{
  int i, pos = -1, best = -1;
  int len = line_len();
  int newcur;

  for(i = 0; i < len; i++){
    if(input_stamp[i] > best){
      best = input_stamp[i];
      pos = i;
    }
  }
  if(pos < 0)
    return 0;

  newcur = curpos;
  if(pos < curpos)
    newcur--;
  return delete_range_at(pos, 1, newcur);
}

static void
move_to_next_word(void)
{
  int len = line_len();

  while(curpos < len && line_get(curpos) != ' ')
    curpos++;
  while(curpos < len && line_get(curpos) == ' ')
    curpos++;
  move_cursor_cga();
}

static void
move_to_prev_word(void)
{
  int i;

  if(curpos <= 0){
    move_cursor_cga();
    return;
  }

  i = curpos - 1;
  if(line_get(i) == ' '){
    while(i > 0 && line_get(i - 1) == ' ')
      i--;
    while(i > 0 && line_get(i - 1) != ' ')
      i--;
  } else {
    while(i > 0 && line_get(i - 1) != ' ')
      i--;
  }
  curpos = i;
  move_cursor_cga();
}

static int
is_printable_input(int c)
{
  return c >= ' ' && c < 0x80 && c != 0x7f;
}

static int
current_word_start(void)
{
  int start = curpos;
  while(start > 0 && line_get(start - 1) != ' ')
    start--;
  return start;
}

static int
find_matching_files(char *prefix, char matches[][MAX_FILENAME_LEN], int max_matches)
{
  char *basic_files[] = {
    "cat", "cd", "echo", "find_sum", "forktest", "grep", "init", "kill",
    "ln", "ls", "mkdir", "rm", "sh", "stressfs", "usertests", "wc", "zombie",
    "README", "result.txt"
  };
  int basic_count = sizeof(basic_files) / sizeof(basic_files[0]);
  int match_count = 0;
  int prefix_len = strlen(prefix);
  int i;

  for(i = 0; i < basic_count && match_count < max_matches; i++){
    if(prefix_len == 0 || strncmp(basic_files[i], prefix, prefix_len) == 0){
      safestrcpy(matches[match_count], basic_files[i], MAX_FILENAME_LEN);
      match_count++;
    }
  }
  return match_count;
}

static void
find_common_prefix(char matches[][MAX_FILENAME_LEN], int match_count, char *common_prefix)
{
  int i, j, common_len;

  if(match_count == 0){
    common_prefix[0] = 0;
    return;
  }

  safestrcpy(common_prefix, matches[0], MAX_FILENAME_LEN);
  common_len = strlen(common_prefix);

  for(i = 1; i < match_count && common_len > 0; i++){
    for(j = 0; j < common_len && j < strlen(matches[i]); j++)
      if(common_prefix[j] != matches[i][j])
        break;
    common_len = j;
  }
  common_prefix[common_len] = 0;
}

static void
autocomplete_at_cursor(void)
{
  char matches[MAX_MATCHES][MAX_FILENAME_LEN];
  char common_prefix[MAX_FILENAME_LEN];
  char current_prefix[MAX_FILENAME_LEN];
  int prefix_len, match_count, start, i;

  start = current_word_start();
  prefix_len = curpos - start;
  if(prefix_len >= MAX_FILENAME_LEN)
    prefix_len = MAX_FILENAME_LEN - 1;
  for(i = 0; i < prefix_len; i++)
    current_prefix[i] = line_get(start + i);
  current_prefix[prefix_len] = 0;

  match_count = find_matching_files(current_prefix, matches, MAX_MATCHES);
  if(match_count == 0)
    return;

  if(match_count == 1){
    delete_range_at(start, curpos - start, start);
    insert_bytes_at(start, matches[0], strlen(matches[0]));
    redraw_input();
    return;
  }

  find_common_prefix(matches, match_count, common_prefix);
  if(strlen(common_prefix) > strlen(current_prefix)){
    delete_range_at(start, curpos - start, start);
    insert_bytes_at(start, common_prefix, strlen(common_prefix));
    redraw_input();
    return;
  }

  consputc('\n');
  for(i = 0; i < match_count; i++){
    char *name = matches[i];
    while(*name)
      consputc(*name++);
    consputc(' ');
    consputc(' ');
    if((i + 1) % 5 == 0)
      consputc('\n');
  }
  consputc('\n');
  input_start_pos = cga_getpos();
  input_drawn_len = 0;
  serial_cursor_pos = 0;
  redraw_input();
}

void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):
      doprocdump = 1;
      break;

    case C('U'):
      if(selection_final()){
        clear_selection_state();
        redraw_input();
        break;
      }
      if(line_len() > 0){
        delete_range_at(0, line_len(), 0);
        clear_selection_state();
        redraw_input();
      }
      break;

    case C('H'): case '\x7f':
      if(selection_final()){
        delete_final_selection();
        redraw_input();
      } else if(curpos > 0){
        delete_range_at(curpos - 1, 1, curpos - 1);
        redraw_input();
      }
      break;

    case C('D'):
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else {
        move_to_next_word();
      }
      break;

    case C('A'):
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else {
        move_to_prev_word();
      }
      break;

    case C('Z'):
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else if(delete_latest_inserted_char()){
        redraw_input();
      }
      break;

    case C('S'):
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else if(!selection_active){
        selection_active = 1;
        selection_start_index = curpos;
        selection_end_index = -1;
      } else {
        int a = selection_start_index;
        int b = curpos;
        selection_active = 0;
        if(a == b){
          clear_selection_state();
        } else {
          if(a < b){
            selection_start_index = a;
            selection_end_index = b;
          } else {
            selection_start_index = b;
            selection_end_index = a;
          }
        }
        redraw_input();
      }
      break;

    case C('C'):
      if(selection_final()){
        copy_final_selection();
        redraw_input();
      }
      break;

    case C('V'):
      if(selected_buffer[0] == 0){
        if(selection_final()){
          clear_selection_state();
          redraw_input();
        }
      } else if(selection_final()){
        replace_final_selection(selected_buffer, strlen(selected_buffer));
        redraw_input();
      } else {
        insert_bytes_at(curpos, selected_buffer, strlen(selected_buffer));
        redraw_input();
      }
      break;

    case LEFT_ARROW:
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else if(curpos > 0){
        curpos--;
        move_cursor_cga();
      }
      break;

    case RIGHT_ARROW:
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else if(curpos < line_len()){
        curpos++;
        move_cursor_cga();
      }
      break;

    case TAB_KEY:
      if(selection_final()){
        clear_selection_state();
        redraw_input();
      } else {
        autocomplete_at_cursor();
      }
      break;

    default:
      c = (c == '\r') ? '\n' : c;
      if(selection_final()){
        if(is_printable_input(c)){
          char ch = c;
          replace_final_selection(&ch, 1);
          redraw_input();
        } else {
          clear_selection_state();
          redraw_input();
        }
        break;
      }

      if(c == '\n'){
        char ch = '\n';
        clear_selection_state();
        curpos = line_len();
        if(insert_bytes_at(curpos, &ch, 1)){
          consputc('\n');
          input.w = input.e;
          reset_line_editor();
          wakeup(&input.r);
        }
      } else if(is_printable_input(c)){
        char ch = c;
        if(insert_bytes_at(curpos, &ch, 1))
          redraw_input();
      }
      break;
    }
  }

  release(&cons.lock);

  if(doprocdump)
    procdump();
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){
      if(n < target){
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}
