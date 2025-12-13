set -e                  # exit on error
set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error

print_ok "Configuring templates..."
mkdir -p /etc/skel/Templates
touch /etc/skel/Templates/Text.txt

touch /etc/skel/Templates/Markdown.md
cat << 'EOF' > /etc/skel/Templates/Markdown.md
# Title

- [ ] Task 1
- [ ] Task 2
- [ ] Task 3

## Subtitle

1. Numbered 1
2. Numbered 2
3. Numbered 3
EOF

touch /etc/skel/Templates/Dart.dart
cat << 'EOF' > /etc/skel/Templates/Dart.dart
void main() {
  // متغير بسيط
  String message = "Hello, Dart!";
  print(message);

  // دالة بسيطة
  int add(int a, int b) {
    return a + b;
  }

  print('2 + 3 = ${add(2, 3)}');
}
EOF

touch /etc/skel/Templates/Python.py
cat << 'EOF' > /etc/skel/Templates/Python.py
# تعريف دالة
def main():
    # طباعة رسالة ترحيب
    print("Hello, Python!")

    # حلقة بسيطة
    for i in range(5):
        print(f"Iteration: {i}")

if __name__ == "__main__":
    main()
EOF

touch /etc/skel/Templates/C.c
cat << 'EOF' > /etc/skel/Templates/C.c
#include <stdio.h>

// الدالة الرئيسية
int main() {
    // طباعة رسالة ترحيب
    printf("Hello, C!\n");

    // تعريف متغير وطباعته
    int number = 10;
    printf("The number is: %d\n", number);

    return 0;
}
EOF

touch /etc/skel/Templates/Cpp.cpp
cat << 'EOF' > /etc/skel/Templates/Cpp.cpp
#include <iostream>
#include <string>

// الدالة الرئيسية
int main() {
    // استخدام مكتبة iostream
    std::cout << "Hello, C++!" << std::endl;

    // استخدام متغير من نوع string
    std::string name = "User";
    std::cout << "Welcome, " << name << "!" << std::endl;

    return 0;
}
EOF

judge "Configure templates"