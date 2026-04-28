# xv6 Login System - Custom Implementation

## Overview

This is a modified version of the xv6 kernel that implements a complete **user authentication and management system**. The login-system branch extends the base xv6 with multi-user support, including user registration, authentication, role-based access control, and secure password hashing.

**Branch:** `login-system`  
**Base:** MIT PDOS xv6 kernel

---

## Key Features Added

### 1. **User Authentication System**
- Login program with username and password verification
- Password hashing using a simple hash function (DJB2 algorithm variant)
- User database stored in a `users` file on the filesystem
- Session-based user tracking

### 2. **User Management Commands**
- **`useradd`** - Create new user accounts with roles
- **`userdel`** - Delete existing user accounts
- **`mkusers`** - Initialize the user database with default users

### 3. **Role-Based Access Control**
- Two role levels: **ADMIN** and **USER**
- Admin-only commands: `useradd`, `userdel`, `mkusers`
- Restricted file access (users file protected from cat command)
- Non-admin users cannot overwrite the users database

### 4. **System Calls**
- **`setuid(int uid)`** - Set the user ID of the current process
- **`getuid()`** - Get the current user ID
- **`clear()`** - Clear the console screen

### 5. **User Structure**
Each user has:
- **username** (16 characters max)
- **passhash** (unsigned int - hashed password)
- **role** (0 = USER, 1 = ADMIN)

---

## New System Calls

### `setuid(int uid)`
Sets the user ID of the current process. Used after successful login to mark the process as running under a specific user account.

**File:** `sysproc.c`  
**Declaration:** `user.h`

### `getuid()`
Returns the user ID of the current process. Used to verify which user is currently active.

**File:** `sysproc.c`  
**Declaration:** `user.h`

### `clear()`
Clears the console screen by printing newlines. Used in the login program for a clean interface.

**File:** `console.c`  
**Declaration:** `user.h`

---

## New User Programs

### `login` (login.c)
The main entry point for user authentication.

**Features:**
- Displays welcome banner
- Prompts for username and password
- Validates credentials against the users database
- Sets the process UID on successful login
- Drops to shell after successful authentication
- Handles multiple login attempts

**Password Validation:**
Compares the hash of entered password with stored password hash using DJB2 hash function.

**Usage:**
```
$ login
Username: admin
Password: ****
(authenticated - shell starts)
```

### `useradd` (useradd.c)
Creates new user accounts in the users database.

**Features:**
- Admin command (restricts to admin users)
- Takes username and password as arguments
- Appends new user to users file
- Prevents overwriting existing users

**Usage:**
```
$ useradd john mypassword
$ useradd alice securepass
```

### `userdel` (userdel.c)
Deletes user accounts from the database.

**Features:**
- Admin command (restricts to admin users)
- Removes specified user from users file
- Rebuilds the users database without the deleted user

**Usage:**
```
$ userdel john
```

### `mkusers` (mkusers.c)
Initializes the user database with default users.

**Default Users Created:**
- `admin` (role: ADMIN) - password hash: hashed "password"
- `user` (role: USER) - password hash: hashed "user"

**Usage:**
```
$ mkusers
```
Creates/initializes the `users` file with default admin and user accounts.

### `clr` (clr.c)
A simple utility program that clears the console screen.

**Features:**
- Calls the `clear()` system call
- Clears the terminal by printing blank lines
- Useful for cleaning up the screen after running commands

**Usage:**
```
$ clr
```
Clears the console and exits.

### `rfl` (rfl.c)
**Read First Line** - A utility to display the first line of files.

**Features:**
- Reads and prints only the first line of a file (up to newline character)
- Supports multiple files as arguments
- Can read from standard input if no files specified
- Exits after printing the first line
- Author: Anthony Badawi

**Usage:**
```
$ rfl /etc/hostname
$ rfl file1.txt file2.txt file3.txt
$ echo "test" | rfl
```

**Examples:**
```
$ rfl users
admin
```
Prints only the first line from the users file.

```
$ cat /etc/hosts | rfl
127.0.0.1
```
Reads from stdin and prints first line.

### `ls` (ls.c) - Wildcard Support
**Enhanced listing utility with wildcard pattern matching**

**Features Added:**
- Supports `*` wildcard pattern matching for filenames
- Automatically detects wildcard characters in arguments
- Expands patterns to match multiple files in the current directory
- Falls back to normal ls behavior for non-wildcard arguments
- Works with the `ls *` pattern to list all files

**Pattern Matching:**
- The `match()` function implements recursive pattern matching
- `*` matches zero or more characters
- Patterns are matched against filenames in the current directory
- Returns matching files with file info (type, inode, size)

**Limitations:**
- **Only works in the current directory** - patterns like `/tmp/*` or `subdir/*` are not supported
- **Current directory only** - the `ls_wildcard()` function is hardcoded to open `.` (dot)
- No support for other glob patterns like `?` or `[...]`
- Shell doesn't expand wildcards, so this implementation handles it in the ls command itself

**Usage:**
```
$ ls                        # Normal listing of current directory
$ ls *                      # Lists all files matching *
$ ls *.c                    # Would match all .c files (if shell expanded)
$ ls file*                  # Lists files starting with "file" in current directory
```

**Examples:**
```
$ ls
.        2 1 512
..       2 1 512
usr      2 4 512
tmp      2 5 512

$ ls *
.           2 1 512
..          2 1 512
cat.c       1 12 1024
echo.c      1 13 2048
```

**Implementation Details:**

The enhancement adds two main components:

1. **Pattern Matching Function (`match()`):**
   ```c
   int match(char *pattern, char *name){
     if(*pattern == 0 && *name == 0)
       return 1;
     if(*pattern == '*'){
       return match(pattern+1, name) || (*name && match(pattern, name+1));
     }
     if(*pattern == *name)
       return match(pattern+1, name+1);
     return 0;
   }
   ```
   Uses recursive backtracking to match `*` wildcard.

2. **Wildcard Handler (`ls_wildcard()`):**
   - Opens the current directory
   - Iterates through directory entries
   - Uses `match()` to check each filename against the pattern
   - Stats matched files and displays their information
   - Constructs full paths using `./filename` format

**Code Location:**
- Pattern matching logic: lines 25-36
- Wildcard listing function: lines 89-123
- Main program detection: lines 135-140 (checks for `*` in arguments)

---

## Modified Files

### 1. **Makefile**
- Added targets for new user programs: `useradd`, `userdel`, `login`, `mkusers`
- Updated build rules to compile and link new programs
- Added `users` file to filesystem image creation

### 2. **syscall.c**
- Added entries for `setuid` and `getuid` system calls
- Registered new system call numbers

### 3. **syscall.h**
- Defined system call numbers: `SYS_setuid`, `SYS_getuid`, `SYS_clear`

### 4. **sysproc.c**
- Implemented `setuid()` - sets process uid field
- Implemented `getuid()` - retrieves process uid field
- Implemented `clear()` - prints newlines to clear screen

### 5. **proc.c**
- Added `uid` field initialization in process creation
- Default uid of 0 for non-logged-in processes

### 6. **proc.h**
- Added `int uid;` field to `struct proc`
- Tracks the user ID for each process

### 7. **user.h**
- Added declarations for `setuid()`, `getuid()`, `clear()`
- Exported new system calls to user space

### 8. **init.c**
- Updated to start login program instead of shell directly
- Enhanced process initialization with user context
- Changed default startup behavior to require login

### 9. **sh.c** (shell)
- Added admin command restrictions in shell:
  - `useradd` restricted to admin users
  - `userdel` restricted to admin users
  - `mkusers` restricted to admin users
- Checks process UID against role requirements before executing restricted commands

### 10. **cat.c**
- Added security check to prevent reading the `users` file
- Direct read attempts of `users` return error
- Protects password database from casual inspection

### 11. **ls.c**
- Added wildcard pattern matching support with `*` character
- Implemented `match()` function for recursive pattern matching
- Added `ls_wildcard()` function to handle wildcard expansions
- Enhanced main program to detect wildcards in arguments
- **Limitation:** Wildcard expansion limited to current directory only

---

## Technical Details

### Password Hashing

The system uses a **DJB2 hash algorithm variant** for password hashing:

```c
unsigned int simple_hash(char *s) {
  unsigned int h = 5381;
  for(int i = 0; s[i]; i++){
    h = ((h << 5) + h) + s[i]; // h * 33 + c
  }
  return h;
}
```

**Note:** This is a non-cryptographic hash suitable for simple authentication. For production systems, use bcrypt, argon2, or similar cryptographic hash functions.

### User Database Format

The `users` file is a binary file containing `struct user` records:

```c
struct user {
  char username[16];      // 16 bytes username
  unsigned int passhash;  // 4 bytes password hash
  int role;               // 4 bytes role (0=USER, 1=ADMIN)
};
```

Total: 24 bytes per user record.

### Process UID Tracking

- Each process (`struct proc`) has a `uid` field
- Default UID is 0 (represents unauthenticated/system processes)
- After successful login, the login process calls `setuid(user_id)` before executing the shell
- Child processes inherit the parent's UID

### Security Measures

1. **Users File Protection:**
   - `cat` command cannot read the `users` file
   - Non-admin users cannot write to `users` file
   - Prevents unauthorized access to password hashes

2. **Role-Based Command Restriction:**
   - Admin commands check `getuid()` to verify admin status
   - Non-admin attempts to use restricted commands fail gracefully

3. **Login Enforcement:**
   - System starts with login program, not shell
   - No direct shell access without authentication

---

## Building and Running

### Build the Modified xv6

```bash
cd /home/anthony/Desktop/xv6-public
make clean
make
```

### Initialize the User Database

Before first boot, create users:

```bash
make qemu-nox
# In the kernel, run:
$ mkusers
$ quit
```

### Login with Credentials

Default test accounts:
- **Username:** `admin` | **Password:** `password`
- **Username:** `user` | **Password:** `user`

### Create Additional Users (Admin Only)

```bash
$ useradd john mypassword
$ useradd alice securepass123
```

### Delete Users (Admin Only)

```bash
$ userdel john
```

---

## Commit History

The implementation was built incrementally across 10 commits:

1. **Add login program** - Initial login functionality with hardcoded users
2. **Implement user ID system** - `setuid` and `getuid` system calls
3. **Restrict admin commands** - Shell-level access control enforcement
4. **Add strncmp** - String comparison utility for authentication
5. **Add useradd command** - Dynamic user creation
6. **Restrict user access** - Protection of users file in cat and shell
7. **Add user deletion** - `userdel` command for user management
8. **Enhance security** - Additional file access restrictions
9. **Prevent filesystem overwrite** - Safeguard user database during filesystem operations

---

## Testing

### Test Admin Access
```bash
$ login
Username: admin
Password: password
$ useradd testuser testpass   # Should succeed
$ userdel testuser             # Should succeed
```

### Test User Access
```bash
$ login
Username: user
Password: user
$ useradd hacker hack          # Should be denied
$ cat users                    # Should be denied
```

### Test Login Failure
```bash
$ login
Username: admin
Password: wrongpass
(Should loop back to login prompt)
```

---

## Limitations and Future Improvements

### Current Limitations
- Password hashing is non-cryptographic (DJB2 hash)
- No password complexity requirements
- No account lockout after failed attempts
- No password expiration or aging
- Users database is text-based binary file (no encryption at rest)
- Simple hash collision potential with weak passwords

### Recommended Improvements
1. Implement proper cryptographic hashing (bcrypt/argon2)
2. Add password policy enforcement
3. Implement account lockout mechanism
4. Add password expiration/reset functionality
5. Encrypt the users database file
6. Add user groups and permissions system
7. Implement sudo/privilege escalation mechanism
8. Add user home directories
9. Implement file ownership and permissions per user
10. Add audit logging for security events

---

## Files Added/Modified Summary

| File      | Type     | Changes                                        |
|-----------|----------|------------------------------------------------|
| login.c   | NEW      | 88 lines - Login program with authentication   |
| useradd.c | NEW      | 57 lines - User creation utility               |
| userdel.c | NEW      | 53 lines - User deletion utility               |
| mkusers.c | NEW      | 46 lines - User database initialization        |
| clr.c     | NEW      | 8 lines - Console clear utility                |
| rfl.c     | NEW      | 42 lines - Read first line of file utility     |
| sysproc.c | MODIFIED | +15 lines - Add setuid/getuid/clear syscalls   |
| syscall.c | MODIFIED | +4 lines - Register new syscalls               |
| syscall.h | MODIFIED | +4 lines - Syscall definitions                 |
| proc.h    | MODIFIED | +1 line - Add uid field to struct proc         |
| proc.c    | MODIFIED | +1 line - Initialize uid in process            |
| user.h    | MODIFIED | +2 lines - Add new syscall declarations        |
| init.c    | MODIFIED | +17 lines - Start login instead of shell       |
| sh.c      | MODIFIED | +30 lines - Enforce admin command restrictions |
| cat.c     | MODIFIED | +6 lines - Protect users file from reading     |
| ls.c      | MODIFIED | +50 lines - Add wildcard pattern matching      |
| Makefile  | MODIFIED | +8 lines - Add new program targets             |

**Total:** 377 lines added across 6 new programs, 54 lines modified across 11 kernel/utility files

---

## Author Notes

This implementation demonstrates:
- Multi-user kernel support
- Process-based user context tracking
- Role-based access control (RBAC)
- User authentication mechanisms
- File system security policies
- Command-level authorization enforcement

The login-system branch provides a foundation for understanding user management in operating systems and could be extended with more sophisticated security features.

---

## References

- **Base xv6:** MIT PDOS Educational OS
- **Branch:** login-system (diverged from master)
- **Implementation Date:** 2026
- **Student Devs:** Anthony Badawi | Moustafa Hoteit
