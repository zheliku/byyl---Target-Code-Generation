int f(int x1, int x2, int x3, int x4) {
    int y = x1 + x2 + x3 + x4;
    return y;
}

int main() {
    int a1, a2, a3, a4, n;
    a1 = read();
    a2 = read();
    a3 = read();
    a4 = read();
    n = f(a1, a2, a3, a4);
    write(n);
    return 0;
}