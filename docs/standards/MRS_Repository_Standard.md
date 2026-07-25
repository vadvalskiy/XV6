# MRAGETSARS Repository Standard

## استاندارد رسمی ساخت، مستندسازی و نگهداری ریپوزیتوری‌ها

| مشخصه | مقدار |
| --- | --- |
| نام کوتاه استاندارد | `MRS-RS` |
| نسخه سند | `0.2.0-draft` |
| وضعیت | پیش‌نویس رسمی در حال تدوین |
| مالک استاندارد | `@mragetsars` |
| دامنه | همه ریپوزیتوری‌های عمومی و خصوصی متعلق به `@mragetsars` |
| زبان اصلی مستندات فنی | انگلیسی |
| شاخه پیش‌فرض استاندارد | `main` |
| قالب نام ریپوزیتوری | `Title-Case-With-Hyphens` |
| آخرین بازنگری | 11 July 2026 |

---

# 1. هدف سند

این سند قواعد یکپارچه طراحی، ساختاربندی، مستندسازی، اعتبارسنجی و نگهداری ریپوزیتوری‌های متعلق به `@mragetsars` را تعریف می‌کند.

اهداف اصلی:

1. ایجاد هویت بصری و مهندسی یکپارچه؛
2. حفظ تفاوت‌های واقعی میان انواع پروژه؛
3. افزایش قابلیت فهم، اجرا، آزمایش و نگهداری؛
4. جلوگیری از ناهمگونی تدریجی نام‌ها و ساختارها؛
5. مشخص‌کردن وضعیت فنی، حقوقی و بازتولیدپذیری؛
6. ایجاد مبنای قابل ممیزی برای اصلاح پروژه‌های موجود؛
7. فراهم‌کردن یک چارچوب پایدار برای پروژه‌های آینده.

هدف استاندارد، یکسان‌کردن مصنوعی همه پروژه‌ها نیست. ساختار آن از یک هسته ثابت، پروفایل‌های تخصصی پروژه و پروفایل‌های Artifact تشکیل می‌شود.

---

# 2. معماری استاندارد

```text
MRS-RS
├── Core Standard
│   ├── Repository identity
│   ├── Naming rules
│   ├── README structure
│   ├── Documentation style
│   ├── Git hygiene
│   ├── Licensing and rights
│   ├── Testing and verification
│   ├── CI conventions
│   ├── Artifact management
│   └── Maintenance rules
│
├── Project Profiles
│   ├── P01 — Production Tool or Library
│   ├── P02 — Software Application or Service
│   ├── P03 — C/C++ Console and System Project
│   ├── P04 — Game and Interactive Application
│   ├── P05 — AI, Machine Learning, Statistics, and Data Science
│   ├── P06 — Data Engineering and Distributed Systems
│   ├── P07 — RTL, Hardware, and Digital Design
│   └── P08 — GitHub Profile Repository
│
└── Artifact Profiles
    └── A01 — Jupyter Notebook
```

هر ریپوزیتوری باید:

1. تمام قواعد هسته ثابت را رعایت کند؛
2. یک پروفایل اصلی پروژه داشته باشد؛
3. در صورت نیاز پروفایل فرعی داشته باشد؛
4. پروفایل‌های Artifact مرتبط را رعایت کند؛
5. تمام استثناها را صریحاً مستند کند.

ترتیب اولویت:

```text
Core Standard
    ↓
Primary Project Profile
    ↓
Secondary Project Profile
    ↓
Applicable Artifact Profiles
    ↓
Documented Project-Specific Exceptions
```

---

# 3. واژگان هنجاری

| واژه | معنای هنجاری |
| --- | --- |
| **الزامی — MUST** | رعایت آن برای انطباق ضروری است |
| **ممنوع — MUST NOT** | انجام آن ناقض استاندارد است |
| **توصیه‌شده — SHOULD** | جز با دلیل مستند باید رعایت شود |
| **توصیه‌نشده — SHOULD NOT** | فقط با دلیل معتبر مجاز است |
| **اختیاری — MAY** | رعایت آن به نیاز پروژه بستگی دارد |

---

# 4. سطوح انطباق

| سطح | عنوان | تعریف |
| --- | --- | --- |
| `L0` | Non-Conformant | قواعد پایه رعایت نشده‌اند |
| `L1` | Core Conformant | هسته ثابت رعایت شده است |
| `L2` | Profile Conformant | هسته و پروفایل تخصصی رعایت شده‌اند |
| `L3` | Verified Conformant | Build، Test یا Verification اثبات شده است |
| `L4` | Release Ready | پروژه آماده انتشار یا استفاده عمومی است |

README زیبا به‌تنهایی برای سطح `L3` کافی نیست.

---

# Part I — Core Standard

# 5. هویت ریپوزیتوری

## 5.1. نام ریپوزیتوری

قالب استاندارد:

```text
Title-Case-With-Hyphens
```

نمونه‌های صحیح:

```text
Mardas-MD2PDF
RISC-V-Pipeline-Processor
SnappFood-Sentiment-CNN
Restaurant-Order-Data-Engineering-Pipeline
```

نمونه‌های نامطلوب:

```text
my_project
AI_CA3_final
project2
Final-Version-New
test-repo
```

قواعد:

1. واژه‌ها با `-` جدا شوند.
2. فاصله، underscore و نقطه در نام ریپوزیتوری ممنوع است.
3. مخفف‌های رسمی مانند `AI`، `ML`، `CNN`، `RTL`، `RISC-V` و `PDF` حفظ شوند.
4. شماره Assignment نباید نام اصلی پروژه باشد.
5. واژه‌های `Final`، `New`، `Latest` و `Updated` استفاده نشوند.
6. `Implementation` فقط در صورت نیاز واقعی استفاده شود.
7. نام بدون بازکردن README ماهیت پروژه را نشان دهد.
8. تغییر نام فقط پس از بررسی لینک‌ها، badgeها، CI و اسکریپت‌ها انجام شود.

## 5.2. Description صفحه GitHub

هر ریپوزیتوری باید یک Description یک‌جمله‌ای، دقیق و ترجیحاً 80 تا 160 نویسه‌ای داشته باشد.

Description باید نوع پروژه، مسئله اصلی و فناوری کلیدی را مشخص کند و از ادعاهای تبلیغاتی اجتناب کند.

## 5.3. Topics

هر ریپوزیتوری عمومی باید 5 تا 12 Topic مرتبط داشته باشد.

Topics باید lowercase، قابل جست‌وجو و مرتبط با فناوری، دامنه و نوع پروژه باشند.

## 5.4. شاخه پیش‌فرض

شاخه پیش‌فرض استاندارد:

```text
main
```

مهاجرت پروژه‌های قدیمی باید پس از بررسی CI، badgeها، Branch Protection، GitHub Pages، Release workflow و لینک‌های خام انجام شود.

## 5.5. نام شاخه‌های کاری

پیشوندهای مجاز:

```text
feature/
fix/
docs/
refactor/
test/
chore/
build/
ci/
release/
hotfix/
```

قالب توصیه‌شده:

```text
type/concise-kebab-case-description
```

---

# 6. فایل‌های پایه

## 6.1. فایل‌های الزامی

هر ریپوزیتوری حاوی کد باید حداقل موارد زیر را داشته باشد:

```text
README.md
.gitignore
.gitattributes
```

هر پروژه اجرایی باید `Makefile` یا Task Runner معادل داشته باشد.

## 6.2. وضعیت حقوقی

هر ریپوزیتوری باید دقیقاً یکی از این وضعیت‌ها را داشته باشد:

1. فایل `LICENSE` معتبر؛
2. اعلام صریح نبود مجوز استفاده مجدد؛
3. `LICENSE` برای کد و `NOTICE.md` برای مواد ثالث.

نبود کامل توضیح حقوقی مجاز نیست.

## 6.3. فایل NOTICE

وجود `NOTICE.md` برای Dataset ثالث، صورت‌مسئله دانشگاهی، Asset خارجی، مدل ازپیش‌آموزش‌دیده، Starter Code، فایل‌های تجاری و مواد دارای مجوز متفاوت الزامی است.

## 6.4. فایل‌های توصیه‌شده

```text
.editorconfig
CONTRIBUTING.md
SECURITY.md
CHANGELOG.md
CITATION.cff
CODE_OF_CONDUCT.md
.env.example
```

این فایل‌ها فقط در صورت داشتن محتوای واقعی اضافه شوند.

---

# 7. ساختار اجباری README

## 7.1. ترتیب پایه

```text
1. Project Title
2. Project Context or Tagline
3. Badges
4. Overview
5. Objectives or Features
6. Architecture or Methodology
7. Repository Structure
8. Getting Started
9. Verification and Testing
10. Results or Demonstration
11. Limitations and Reproducibility
12. Author or Contributors
13. License and Data Rights
```

پروفایل تخصصی می‌تواند بخش اضافه کند، اما ترتیب بخش‌های مشترک را بدون دلیل تغییر نمی‌دهد.

## 7.2. عنوان پروژه

README باید دقیقاً یک `H1` داشته باشد:

```markdown
# Project Name
```

Headingهای استاندارد بدون Emoji و با Sentence Case نوشته شوند.

## 7.3. خط زمینه

پروژه دانشگاهی:

```markdown
> **Course Name — University of Tehran — Department Name**
```

پروژه غیردانشگاهی:

```markdown
> **A concise technical description of the project's primary purpose**
```

## 7.4. Badgeها

تعداد badgeها 3 تا 5 و ترتیب آن‌ها چنین است:

```text
Language
Primary Framework or Tool
Status
CI or Verification
License
```

وضعیت‌های مجاز:

```text
Stable
Completed
Completed with Limitations
In Progress
Experimental
Archived
```

License badge فقط در صورت وجود مجوز معتبر نمایش داده شود.

## 7.5. Overview

Overview باید روشن کند:

1. پروژه چیست؛
2. مسئله را چگونه حل می‌کند؛
3. وضعیت و دامنه فعلی چیست.

حجم توصیه‌شده 2 تا 4 پاراگراف کوتاه است. تکرار Features، تاریخچه طولانی و ادعای اثبات‌نشده مجاز نیست.

## 7.6. Objectives یا Features

| نوع پروژه | عنوان |
| --- | --- |
| پژوهشی، دانشگاهی، ML و RTL | `Objectives` |
| ابزار یا کتابخانه | `Features` یا `Objectives` |
| بازی و برنامه تعاملی | `Features` |
| سامانه چندجزئی | `Objectives` |

فقط قابلیت‌های موجود و قابل اثبات درج شوند. برنامه‌های آینده در `Roadmap` قرار گیرند.

## 7.7. Architecture یا Methodology

| ماهیت پروژه | عنوان |
| --- | --- |
| نرم‌افزار، سامانه، RTL، Data Engineering | `Architecture` |
| AI، ML، Statistics، Data Science | `Methodology` |
| پروژه ترکیبی | `Architecture and Methodology` |

این بخش باید مؤلفه‌ها، Interfaceها، الگوریتم، جریان داده یا مراحل اصلی را توضیح دهد.

## 7.8. Repository Structure

وجود این بخش در تمام ریپوزیتوری‌های غیرساده الزامی است.

این بخش یک **نقشه معماری توضیح‌دار** است، نه خروجی خام فرمان `tree`.

### 7.8.1. اهداف

بخش باید مشخص کند:

1. کد اصلی کجاست؛
2. Entry pointها کدام‌اند؛
3. مسئولیت ماژول‌های اصلی چیست؛
4. تست، اسناد، داده و Artifact کجا هستند؛
5. تنظیمات Build، Packaging و CI کجا هستند؛
6. کدام مسیرها برای فهم معماری ضروری‌اند.

### 7.8.2. ریشه درخت

ریشه باید یکی از این دو حالت باشد:

```text
<repository-root>/
```

یا نام دقیق GitHub repository:

```text
Mardas-MD2PDF/
```

نام پوشه محلی، نام ZIP، Assignment code یا نام قدیمی ممنوع است.

### 7.8.3. سطح جزئیات

درخت باید شامل موارد مرتبط زیر باشد:

- پوشه‌های معماری Root؛
- Source package؛
- ماژول‌های دارای مسئولیت مستقل؛
- CLI، GUI، API و Service entry pointها؛
- Tests؛
- Documentation؛
- Scripts؛
- Data، Models، Artifacts و Examples؛
- CI و Release workflows؛
- فایل‌های Metadata و Packaging.

برای پروژه کوچک و متوسط، تمام فایل‌های First-party در Source اصلی نمایش داده شوند.

برای پروژه بزرگ، تمام ماژول‌های معماری و Public entry pointها نمایش داده شوند.

### 7.8.4. توضیح Inline

هر مسیر مهم باید توضیح مسئولیت داشته باشد.

نامناسب:

```text
├── renderer.py         # Python file
├── tests/              # Test folder
└── README.md           # README file
```

مناسب:

```text
├── renderer.py         # HTML assembly, MathJax handling, and Chromium PDF export
├── tests/              # Automated tests for parsing, rendering, CLI, and security boundaries
└── README.md           # Project overview, setup instructions, and documentation entry point
```

### 7.8.5. عمق

عمق توصیه‌شده 2 تا 4 سطح است. عمق بیشتر فقط وقتی مجاز است که بدون آن مرز زیرسامانه مشخص نشود.

### 7.8.6. ترتیب نمایش

ترتیب منطقی پیشنهادی:

```text
Source or package
Interfaces and entry points
Tests
Documentation
Examples
Scripts
Assets
Data
Models and artifacts
CI workflows
Build and package metadata
License and repository documentation
```

### 7.8.7. استفاده از `...`

`...` فقط وقتی مجاز است که:

- فایل‌های مشابه بسیار زیاد باشند؛
- چند نمونه یا الگوی نام‌گذاری نشان داده شده باشد؛
- مسئولیت گروه مشخص باشد؛
- معماری پنهان نشود.

### 7.8.8. مسیرهای ممنوع

این موارد نباید به‌عنوان معماری اصلی نمایش داده شوند:

```text
__pycache__/
.ipynb_checkpoints/
.venv/
build/
dist/
obj/
objects/
work/
node_modules/
coverage/
temporary logs
locally generated executables
editor metadata
```

### 7.8.9. نگهداری

درخت باید در همان Commit یا PR مربوط به تغییر معماری به‌روزرسانی شود.

مسیر ناموجود، تکراری یا قدیمی ناقض استاندارد است.

### 7.8.10. نمونه سطح مطلوب

```text
Mardas-MD2PDF/
├── src/mardas_md2pdf/      # Python package source
│   ├── markdown.py         # Markdown parsing, front matter, TOC, math, Mermaid, footnotes, and safe HTML
│   ├── mermaid.py          # Offline Mermaid flowchart-subset-to-SVG renderer
│   ├── renderer.py         # HTML assembly, appearance CSS, MathJax, and Chromium PDF rendering
│   ├── references.py       # Numbered objects, semantic labels, cross-references, and generated lists
│   ├── citations.py        # Offline BibTeX/CSL JSON parsing, citation resolution, and bibliography output
│   ├── book.py             # Ordered chapter manifest, namespacing, cross-links, and book assembly
│   ├── cli.py              # Conversion command-line interface
│   ├── config.py           # Versioned mardas.toml discovery, validation, and resolution
│   ├── diagnostics.py      # Stable text and JSON diagnostic records
│   ├── project_commands.py # Project diagnostics and Book Mode workflows
│   ├── workspace.py        # Safe Studio project tree, file I/O, diagnostics, preview, and Book export
│   ├── render_pool.py      # Bounded workers with reusable thread-affine Chromium sessions
│   ├── studio_jobs.py      # Disk-backed export jobs, progress, cancellation, and retention
│   ├── gui.py              # Local browser-based GUI backend
│   └── assets/             # Style CSS, GUI shell, logo, and vendored MathJax files
├── docs/                   # Guides, changelog, release, maintenance, security, and policy
│   └── guides/             # Complete English and Persian user guides
├── examples/               # Generated PDF examples from official guides
├── scripts/                # Checks, distributions, visual QA, and cleanup helpers
├── tests/                  # Automated pytest test suite
├── pyproject.toml          # Package metadata, dependencies, and tool configuration
├── .github/workflows/      # CI and release artifact automation
├── LICENSE                 # MIT license
└── README.md               # Project introduction and documentation entry point
```

### 7.8.11. اصل نهایی

> درخت باید به‌اندازه‌ای کامل باشد که معماری پروژه را توضیح دهد، و به‌اندازه‌ای فشرده باشد که قابل خواندن باقی بماند.

### 7.8.12. چک‌لیست

- [ ] نام Root صحیح است.
- [ ] تمام پوشه‌های معماری اصلی نمایش داده شده‌اند.
- [ ] ماژول‌های اصلی Source نمایش داده شده‌اند.
- [ ] Entry pointها مشخص هستند.
- [ ] نقش هر مسیر مهم توضیح داده شده است.
- [ ] Tests، Documentation و Scripts مشخص‌اند.
- [ ] فایل‌های مهم Root نمایش داده شده‌اند.
- [ ] Generated artifactها حذف یا توجیه شده‌اند.
- [ ] مسیر تکراری یا ناموجود وجود ندارد.
- [ ] `...` معماری را پنهان نکرده است.
- [ ] درخت با ساختار فعلی منطبق است.

## 7.9. Getting Started

باید شامل:

```text
Prerequisites
Installation
Configuration
Run
```

فرمان‌ها باید قابل Copy/Paste، مبتنی بر Root و فاقد Placeholder حل‌نشده باشند.

## 7.10. Verification and Testing

README باید Build، Test، Smoke Test، Verification، Benchmark و Visual QA را از هم تفکیک کند و برای هر کدام فرمان و معیار موفقیت ارائه دهد.

## 7.11. Results

منشأ نتیجه باید یکی از این موارد باشد:

```text
Verified
Artifact-derived
Originally reported
External-data required
Illustrative
```

نتیجه بازتولیدنشده نباید Verified معرفی شود.

## 7.12. Limitations and Reproducibility

باید Dataset خارجی، GPU، سیستم‌عامل، نرم‌افزار تجاری، Seed، محدودیت حافظه، Known issue و تفاوت Smoke-tested با Full-tested را توضیح دهد.

## 7.13. Author و Contributors

پروژه فردی:

```markdown
## Author
```

پروژه تیمی:

```markdown
## Contributors
```

نام و لینک Contributorها باید در همه پروژه‌ها ثابت باشد.

## 7.14. License and Data Rights

وضعیت Source code، Documentation، Dataset، Assignment specification، Asset، Font، Model، Checkpoint و Starter code باید روشن باشد.

---

# 8. سبک نگارش و Markdown

## 8.1. زبان

README عمومی به انگلیسی نوشته می‌شود. نسخه فارسی در صورت نیاز:

```text
README.fa.md
```

## 8.2. Headingها

Headingهای استاندارد بدون Emoji، کوتاه و با Sentence Case باشند.

## 8.3. Code Blockها

تمام Code Blockها باید Language Identifier داشته باشند.

## 8.4. نام فایل و کد

نام فایل، مسیر، فرمان و Identifier با backtick نوشته شود.

## 8.5. تصاویر

تصویر باید Alt Text، مسیر نسبی، نام تمیز و اندازه مناسب داشته باشد.

مسیر پیشنهادی:

```text
assets/readme/
```

## 8.6. لینک‌ها

لینک داخلی نسبی باشد. لینک شکسته، تصویر مفقود و Anchor نامعتبر مجاز نیست.

## 8.7. ادعاهای فنی

واژه‌هایی مانند `secure`، `stable`، `production-ready` و `fully tested` فقط همراه با شواهد مجازند.


# 9. ساختار پوشه‌های مشترک

## 9.1. نام پوشه‌ها

برای پروژه‌های جدید، نام پوشه‌ها باید lowercase، بدون فاصله و با نام ساده یا `snake_case` باشند.

نام‌های استاندارد:

```text
src/
include/
tests/
docs/
scripts/
assets/
data/
notebooks/
reports/
outputs/
examples/
artifacts/
config/
```

ساختارهای زیر در پروژه جدید توصیه نمی‌شوند:

```text
Source/
Includes/
Description/
Project/
Builds/
My Files/
Final Report/
```

پروژه قدیمی باید مرحله‌ای مهاجرت داده شود و نباید صرفاً برای ظاهر شکسته شود.

## 9.2. Root Repository

Root باید خلوت باقی بماند.

فایل‌های قابل قبول در Root:

```text
README.md
LICENSE
NOTICE.md
Makefile
pyproject.toml
requirements.txt
docker-compose.yml
.gitignore
.gitattributes
.editorconfig
```

Dataset، گزارش، تصویر و Artifactهای متعدد نباید بدون ساختار در Root قرار گیرند.

---

# 10. Git Hygiene

## 10.1. فایل‌های ممنوع

```text
Virtual environments
Build directories
Object files
Locally generated executables
IDE settings
OS metadata
Caches
Temporary files
Runtime logs
Secrets
.env files
Sensitive database dumps
Large reproducible outputs
Notebook checkpoints
```

نمونه `.gitignore`:

```gitignore
.venv/
__pycache__/
.ipynb_checkpoints/
build/
dist/
obj/
objects/
work/
*.o
*.out
*.exe
.env
*.log
```

## 10.2. Generated Artifactها

Generated artifact فقط در شرایط زیر Commit می‌شود:

1. برای بررسی علمی یا فنی ضروری باشد؛
2. تولید مجدد بسیار پرهزینه باشد؛
3. ابزار تولید عمومی نباشد؛
4. نمونه رسمی Release باشد؛
5. Reference output باشد.

علت نگهداری باید در README توضیح داده شود.

## 10.3. فایل‌های بزرگ

فایل بزرگ باید از GitHub Release، Git LFS، منبع خارجی، Download script یا نمونه کوچک جایگزین استفاده کند.

Dataset، Model یا Binary بزرگ بدون توضیح نباید Commit شود.

## 10.4. Line Endings

فایل `.gitattributes` باید Line Ending و فایل‌های Binary را کنترل کند.

```gitattributes
* text=auto
*.py text eol=lf
*.c text eol=lf
*.cpp text eol=lf
*.h text eol=lf
*.hpp text eol=lf
*.v text eol=lf
*.sv text eol=lf
*.sh text eol=lf
*.md text eol=lf
*.png binary
*.jpg binary
*.pdf binary
```

---

# 11. استاندارد Commit

## 11.1. قالب پیام

```text
type(scope): concise imperative summary
```

Typeهای مجاز:

```text
feat
fix
docs
refactor
test
chore
build
ci
perf
release
revert
```

نمونه‌ها:

```text
docs(readme): standardize repository structure section
fix(renderer): preserve RTL direction in nested tables
test(cli): add invalid input regression cases
ci(python): test supported Python versions
chore(repo): remove generated build artifacts
```

## 11.2. قواعد Commit

هر Commit باید:

- یک هدف مشخص داشته باشد؛
- قابل بررسی باشد؛
- پروژه را خراب رها نکند؛
- پیام روشن داشته باشد؛
- اصلاحات نامرتبط را ترکیب نکند.

پیام‌هایی مانند `update`، `changes`، `fix stuff` و `final` مناسب نیستند.

---

# 12. قرارداد Task Runner

معنای targetهای پایه:

| Target | معنای استاندارد |
| --- | --- |
| `make help` | نمایش فرمان‌ها |
| `make setup` | آماده‌سازی محیط |
| `make build` | Build پروژه |
| `make run` | اجرای حالت اصلی |
| `make test` | تست‌های سریع |
| `make verify` | بررسی کامل‌تر صحت |
| `make lint` | تحلیل استاتیک |
| `make format` | قالب‌بندی |
| `make clean` | حذف خروجی‌های تولیدشده |
| `make all` | Build و Verification اصلی |

Targetهای تخصصی مجاز:

```text
benchmark
notebook
train
evaluate
synthesis
simulation
streaming
release
visual-qa
```

`make clean` نباید Source، Dataset اصلی، Artifact مرجع یا تنظیمات کاربر را حذف کند.

حذف کامل باید فرمان صریح مانند `make clean-all` یا `make purge` داشته باشد.

---

# 13. تست و Verification

## 13.1. حداقل الزامات

هر پروژه اجرایی حداقل یکی از موارد زیر را دارد:

- Unit test؛
- Integration test؛
- Regression test؛
- Smoke test؛
- Simulation testbench؛
- Reference-output comparison؛
- Notebook execution verification.

معیار موفقیت باید صریح باشد.

## 13.2. تکرارپذیری

پروژه تصادفی باید Seed، نسخه Dependencyها، سخت‌افزار مؤثر، تعداد تکرار و دامنه عدم قطعیت را مشخص کند.

Seed به‌تنهایی بازتولیدپذیری کامل را تضمین نمی‌کند.

## 13.3. تست سریع و سنگین

### Fast checks

- در Push و Pull Request اجرا شوند؛
- زمان محدود داشته باشند؛
- Dependency سنگین نداشته باشند.

### Full verification

- دستی، زمان‌بندی‌شده یا هنگام Release اجرا شود؛
- می‌تواند Training، Visual QA، Benchmark یا Integration کامل داشته باشد.

---

# 14. Continuous Integration

## 14.1. مسیر Workflow

```text
.github/workflows/ci.yml
```

Trigger پیشنهادی:

```yaml
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
  workflow_dispatch:
```

## 14.2. Permission

اصل کمترین دسترسی:

```yaml
permissions:
  contents: read
```

Permission بیشتر فقط برای Workflowهایی مانند Release مجاز است.

## 14.3. قواعد CI

CI باید:

- محیط تمیز بسازد؛
- Dependency نصب کند؛
- Build یا Syntax check اجرا کند؛
- Fast tests را اجرا کند؛
- شکست را مخفی نکند.

CI نباید Training بسیار طولانی یا Artifact بی‌ارزش را در هر Push تولید کند.

CI badge فقط برای Workflow پایدار و معنادار نمایش داده شود.

---

# 15. مدیریت Dependency

## 15.1. Python Package

منبع اصلی ترجیحی:

```text
pyproject.toml
```

باید Metadata، نسخه Python، Runtime dependency، Development dependency، Test و Lint configuration را پوشش دهد.

## 15.2. Notebook-Centric

حداقل یکی از موارد زیر:

```text
requirements.txt
requirements.in + requirements.lock
environment.yml
pyproject.toml
```

نسخه Python یا R باید مستند شود.

## 15.3. Dependency خارجی

برای Dataset، Model یا فایل خارجی ثبت شود:

- نام؛
- منبع؛
- نسخه؛
- مسیر مورد انتظار؛
- حجم؛
- مجوز؛
- checksum در صورت اهمیت؛
- فرمان Download در صورت امکان.

Placeholder خالی که شبیه Dependency واقعی است مجاز نیست.

---

# 16. داده، مدل و Artifact

## 16.1. ساختار داده

```text
data/
├── raw/
├── processed/
├── external/
└── README.md
```

`data/README.md` باید منشأ، مجوز و نحوه تهیه داده را توضیح دهد.

## 16.2. مدل و Checkpoint

برای Checkpoint باید ثبت شود:

- معماری؛
- Dataset؛
- نسخه کد؛
- Metric؛
- Seed؛
- فرمت؛
- روش Load؛
- وضعیت حقوقی.

مدل بزرگ باید در Release یا فضای خارجی قرار گیرد.

## 16.3. Notebook Output

Notebook باید دقیقاً یکی از سیاست‌های زیر را انتخاب کند:

- `Clean Notebook Policy`
- `Verified Output Policy`

قواعد کامل در `MRS_Jupyter_Notebook_Standard.md` تعریف می‌شوند.

---

# 17. امنیت و اطلاعات حساس

موارد ممنوع:

```text
API keys
Access tokens
Passwords
Private certificates
Private datasets
Session secrets
.env
Cloud credentials
Personal identifiers
```

فایل نمونه:

```text
.env.example
```

مقادیر آن باید غیرحساس باشند.

پروژه دریافت‌کننده ورودی کاربر، شبکه، فایل یا HTML باید مرزهای امنیتی خود را مستند کند.

واژه `secure` بدون Threat Model یا شواهد فنی مجاز نیست.

---

# 18. نگهداری و وضعیت پروژه

## 18.1. Status

Status باید واقعی باشد. پروژه ناقص و متوقف‌شده نباید `Completed` معرفی شود.

## 18.2. Archive

ریپوزیتوری در صورت توقف نگهداری، جایگزینی، تاریخی‌شدن یا غیرقابل‌اجرابودن بدون برنامه اصلاح باید Archived شود.

README قبل از Archive وضعیت را توضیح دهد.

## 18.3. Roadmap

فقط برنامه‌های واقعی درج شوند. موارد انجام‌شده به Changelog منتقل شوند.

## 18.4. Versioning

Semantic Versioning فقط برای پروژه قابل انتشار استفاده شود:

```text
MAJOR.MINOR.PATCH
```

پروژه دانشگاهی ساده نباید نسخه مصنوعی داشته باشد.

---

# 19. متادیتای صفحه GitHub

موارد قابل ممیزی:

```text
Repository description
Topics
Website or demo URL
Social preview image
Default branch
License detection
Releases
Issues state
Discussions state
Archived state
```

Social preview باید واضح، کم‌متن، دارای نام پروژه و از نظر حقوقی مجاز باشد.

---

# 20. استثناها

استثنا در یکی از این مسیرها ثبت شود:

```text
README.md
docs/maintenance.md
NOTICE.md
```

هر استثنا باید شامل:

1. قاعده نقض‌شده؛
2. دلیل فنی یا حقوقی؛
3. اثر؛
4. برنامه اصلاح یا دائمی‌بودن.

«قدیمی‌بودن پروژه» به‌تنهایی دلیل کافی نیست.

---

# 21. فرآیند مهاجرت

## مرحله 1 — Inventory

- ثبت فایل‌ها و پوشه‌ها؛
- شناسایی Build artifact؛
- بررسی Dependency؛
- بررسی License و Data؛
- ثبت فرمان اجرا؛
- بررسی لینک و تصویر.

## مرحله 2 — Documentation Baseline

- اصلاح README؛
- استانداردسازی Heading؛
- اصلاح Repository Structure؛
- ثبت Limitations؛
- ثبت وضعیت حقوقی.

## مرحله 3 — Git Controls

- اصلاح `.gitignore`؛
- افزودن `.gitattributes`؛
- حذف Secret؛
- بررسی فایل بزرگ؛
- پاک‌سازی Artifact.

## مرحله 4 — Execution Contract

- استانداردسازی Makefile؛
- تعریف `setup`، `test`، `verify` و `clean`؛
- ثبت Environment؛
- افزودن Smoke Test.

## مرحله 5 — CI

- افزودن Workflow پایه؛
- اجرای Build یا Test در محیط تمیز؛
- اصلاح شکست‌ها؛
- افزودن badge پس از پایداری.

## مرحله 6 — Structural Migration

- تغییر نام پوشه؛
- انتقال اسناد و تصاویر؛
- اصلاح Import و Script؛
- حفظ Compatibility در صورت نیاز.

## مرحله 7 — Branch Migration

- بررسی ارجاعات؛
- مهاجرت به `main`؛
- اصلاح Workflow و badge؛
- فعال‌سازی Branch Protection.

## مرحله 8 — Final Verification

- Clone تمیز؛
- Setup؛
- Build؛
- Test؛
- Verification؛
- بررسی README؛
- بررسی Git status؛
- ثبت نتیجه ممیزی.

---

# 22. چک‌لیست انطباق هسته

## Repository Identity

- [ ] نام مطابق استاندارد است.
- [ ] Description دقیق است.
- [ ] Topics ثبت شده‌اند.
- [ ] شاخه `main` است یا مهاجرت مستند شده است.
- [ ] Status واقعی است.

## Root Files

- [ ] `README.md` وجود دارد.
- [ ] `.gitignore` وجود دارد.
- [ ] `.gitattributes` وجود دارد.
- [ ] License روشن است.
- [ ] در صورت نیاز `NOTICE.md` وجود دارد.
- [ ] Task Runner وجود دارد.

## README

- [ ] یک `H1` صحیح دارد.
- [ ] Context line وجود دارد.
- [ ] Badgeها محدود و واقعی‌اند.
- [ ] Overview تکراری نیست.
- [ ] Features یا Objectives واقعی‌اند.
- [ ] Architecture یا Methodology روشن است.
- [ ] Repository Structure واقعی و توضیح‌دار است.
- [ ] Entry pointها و ماژول‌های اصلی مشخص‌اند.
- [ ] Getting Started قابل اجرا است.
- [ ] Verification دارای فرمان و معیار موفقیت است.
- [ ] منشأ Results روشن است.
- [ ] Limitations صریح‌اند.
- [ ] Contributorها صحیح‌اند.
- [ ] License and Data Rights روشن است.

## Engineering Hygiene

- [ ] Artifact بدون دلیل در Git نیست.
- [ ] Secret وجود ندارد.
- [ ] Dependency مشخص است.
- [ ] Environment version ثبت شده است.
- [ ] Output policy مشخص است.
- [ ] Test یا Smoke Test وجود دارد.
- [ ] `clean` مخرب نیست.
- [ ] Case نام‌ها ثابت است.

## Verification

- [ ] پروژه در محیط تمیز اجرا یا Build شده است.
- [ ] Fast tests موفق‌اند.
- [ ] فرمان‌های README بررسی شده‌اند.
- [ ] لینک و تصویر سالم‌اند.
- [ ] Results با شواهد منطبق‌اند.
- [ ] Git working tree پس از Verification تمیز است.

---

# Part II — Project Profiles

## P01 — Production Tool or Library

برای Package، CLI، GUI utility، پروژه Versioned و Release-oriented.

مرجع: `Mardas-MD2PDF`

## P02 — Software Application or Service

برای Application چندلایه، Web، API، Authentication و State.

مرجع: `University-Teaching-Management-System`

## P03 — C/C++ Console and System Project

برای Terminal، System programming، IPC و پروژه Makefile-based.

مراجع: `TETRIS`، `Terminal-Based-Full-Duplex-Chat`، `Linux-Tree`

## P04 — Game and Interactive Application

برای SFML، Desktop، Terminal game، Assets، Controls و Screenshot.

مراجع: `TETRIS`، `Plants-VS-Zombies`، `Stardew-Valley-Mini-Game`

## P05 — AI, Machine Learning, Statistics, and Data Science

برای ML، DL، AI، آمار، Notebook و پروژه آزمایش‌محور.

مراجع: `Pac-Man-Search-Algorithms`، `UT-Match-Machine-Learning-Services`، `SnappFood-Sentiment-CNN`، `Applied-Predictive-Modeling-And-Ranking`، `Deep-Learning-Workflows-for-Data-Science`

## P06 — Data Engineering and Distributed Systems

برای Kafka، Spark، Database، ETL/ELT، Streaming و Docker.

مرجع: `Restaurant-Order-Data-Engineering-Pipeline`

## P07 — RTL, Hardware, and Digital Design

برای Verilog، Processor، Datapath، Controller، Testbench، Simulation، Synthesis و Timing.

مراجع: `RISC-V-Pipeline-Processor`، `MIPS-MultiCycle-Processor`، `LIF-Spiking-Neuron-RTL-Design`، `Bit-Serial-Matrix-Vector-Multiplier`

## P08 — GitHub Profile Repository

برای `mragetsars/mragetsars` و قواعد معرفی حرفه‌ای، Skills، پروژه‌های منتخب، آمار و ارتباط.

---

# Part III — Artifact Profiles

## A01 — Jupyter Notebook

برای تمام فایل‌های `*.ipynb`.

قواعد کامل در:

```text
MRS_Jupyter_Notebook_Standard.md
```

---

# 23. تصمیم‌های تثبیت‌شده نسخه 0.2

| موضوع | تصمیم |
| --- | --- |
| شاخه پیش‌فرض | `main` |
| نام ریپوزیتوری | `Title-Case-With-Hyphens` |
| نام پوشه داخلی | lowercase |
| Heading استاندارد | بدون Emoji |
| زبان README | انگلیسی |
| نسخه فارسی | `README.fa.md` |
| تعداد badge | حداکثر 5 |
| Task Runner ترجیحی | `Makefile` |
| License | صریح |
| مواد ثالث | `NOTICE.md` |
| Build artifact | خارج از Git مگر با دلیل |
| تست سنگین | جدا از CI سریع |
| نتیجه ML | همراه با provenance |
| Contributor | املای ثابت |
| Repository Tree | نقشه معماری توضیح‌دار |
| Status vocabulary | محدود به مقادیر رسمی |
| Notebook | تابع پروفایل `A01` |

---

# 24. تاریخچه نسخه

## `0.2.0-draft`

- افزودن Artifact Profiles؛
- افزودن `A01` برای Notebook؛
- بازنویسی تفصیلی Repository Structure؛
- افزودن سطح جزئیات، عمق، ترتیب، Inline description و checklist؛
- به‌روزرسانی چک‌لیست انطباق.

## `0.1.0-draft`

- ایجاد Core + Project Profiles؛
- تعریف واژگان هنجاری و سطوح انطباق؛
- تعریف README، Git، CI، Testing، Dependency، License و Migration.
