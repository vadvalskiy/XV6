# آزمایشگاه‌های سیستم‌عامل xv6

> **آزمایشگاه سیستم‌عامل — دانشگاه تهران — دانشکده مهندسی برق و کامپیوتر**

## معرفی

این ریپوزیتوری شامل **یک نسخهٔ تجمعی و واحد از xv6/x86** است. چهار آزمایشگاه به‌صورت سری روی یک درخت source اعمال شده‌اند؛ یعنی Lab 2 ادامهٔ Lab 1، Lab 3 ادامهٔ Labs 1–2 و Lab 4 ادامهٔ Labs 1–3 است. فایل‌های اجرایی و source نهایی مستقیماً در ریشهٔ ریپوزیتوری قرار دارند و هیچ کپی مستقل و موازی برای هر Lab وجود ندارد.

صورت‌مسئله‌ها و گزارش‌های اولیه در `docs/labs/` صرفاً برای حفظ provenance و مستندات آموزشی نگهداری شده‌اند. این فایل‌ها source جایگزین نیستند و خروجی‌های تاریخی آن‌ها بدون اجرای مجدد، «تأییدشده» معرفی نمی‌شوند.

## مسیر تکامل تجمعی

| مرحله | قابلیت افزوده‌شده به همان هسته | برنامهٔ بررسی اصلی |
| --- | --- | --- |
| پایه | هستهٔ آموزشی MIT xv6/x86 | `usertests` |
| Lab 1 | معرفی اعضا، ویرایش تعاملی کنسول و `find_sum` امن‌تر | `lab1test` |
| Lab 2 | syscallهای تصادفی، اطلاعات پردازه و مرتب‌سازی kernel/user | `lab2test` |
| Lab 3 | scheduler سه‌صفی، aging، workload قطعی و آمار زمان‌بندی | `schedverify` |
| Lab 4 | شمارنده‌های syscall، producer-consumer، RW-lock و ticket-lock | `scounttest`, `pctest`, `rwtest`, `tickettest` |

```text
xv6 پایه
  -> Lab 1
  -> Lab 2
  -> Lab 3
  -> Lab 4
  -> یک kernel و filesystem image نهایی
```

## ساختار اصلی

```text
Xv6-Operating-System-Labs/
├── *.c, *.h, *.S           # هسته و برنامه‌های user تجمعی xv6 در ریشه
├── Makefile                # build، QEMU، dist و verification پروژه
├── docs/labs/              # صورت‌مسئله و گزارش آرشیوی هر فاز
├── docs/                   # معماری، verification و مستندات نگهداری
├── scripts/                # build verification، QEMU smoke و بررسی تاریخچه
├── tests/                  # تست‌های سریع ساختار ریپوزیتوری
├── .github/workflows/      # CI روزمره و ماتریس کامل آزمایش‌ها
├── NOTICE.md               # وضعیت حقوقی مواد ثالث و آموزشی
└── README.md               # مستند فنی اصلی به زبان انگلیسی
```

## پیش‌نیاز و اجرا

روی Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install --yes build-essential gcc-multilib make perl python3 git qemu-system-x86
```

سپس:

```bash
git clone https://github.com/mragetsars/Xv6-Operating-System-Labs.git
cd Xv6-Operating-System-Labs
make setup
make build
make run CPUS=2
```

داخل shell سیستم‌عامل:

```text
lab1test
lab2test
schedverify
scounttest 8 10000
pctest
rwtest
tickettest
```

## بررسی صحت

```bash
make lint                  # ساختار MRS-RS و attribution تاریخچه
make test                  # تست‌های سریع host-side
make verify-build          # build تمیز source، dist و هر سه حالت counter
make smoke CPUS=2          # اجرای همه PASS markerها در یک بوت QEMU
make counter-matrix        # سه mode شمارنده روی CPUS=1 و CPUS=4
```

در زمان بازسازی ریپوزیتوری، build تمام مراحل سری، نسخهٔ نهایی، `dist` و سه mode شمارنده موفق بوده است. QEMU در محیط بازسازی نصب نبود؛ بنابراین runtime داخل guest به‌طور محلی اجرا نشده و برای GitHub Actions آماده شده است. جزئیات در [`docs/verification.md`](docs/verification.md) ثبت شده‌اند.

## محدودیت‌ها

- پروژه برای xv6-public مبتنی بر x86 است، نه xv6-riscv.
- toolchain با قابلیت تولید کد ۳۲ بیتی لازم است.
- زمان‌سنجی مبتنی بر tick دقت محدود دارد و benchmark معتبر به تکرار و نگهداری raw log نیاز دارد.
- حالت `SYSCALL_COUNT_MODE=0` عمداً بدون قفل و دارای race condition است.
- ترتیب دقیق scheduler در حالت چندپردازنده قطعی نیست.
- فایل‌های صورت‌مسئله و گزارش‌ها تحت مجوز MIT کد قرار نمی‌گیرند؛ `NOTICE.md` را ببینید.

## تاریخچه Git

تاریخچه از xv6 پایه آغاز شده و چهار Lab را به‌ترتیب روی همان درخت اعمال می‌کند. زمان‌های تاریخی ساختگی ایجاد نشده‌اند. در تمام commitها یک نفر author اصلی و دو نفر دیگر `Co-authored-by` هستند و سهم author اصلی بین سه عضو برابر است.

## مشارکت‌کنندگان

- [Meraj Rastegar](https://github.com/mragetsars) — `mragetsars@gmail.com` — `@mragetsars`
- [Meraj PourHosseiny](https://github.com/MerajPoorhosseiny) — `meraj.prhosseiny@ut.ac.ir` — `@MerajPoorhosseiny`
- [Ali Sadeghi](https://github.com/Alisssaaaddd) — `ali.sadeghi.m@ut.ac.ir` — `@Alisssaaaddd`

## مجوز و حقوق مواد

کد xv6 و تغییرات source این ریپوزیتوری تحت مجوز MIT در `LICENSE` ارائه می‌شوند. صورت‌مسئله‌ها، گزارش‌های آرشیوی و سند استاندارد ریپوزیتوری دارای وضعیت حقوقی جداگانه‌اند که در `NOTICE.md` توضیح داده شده است.
