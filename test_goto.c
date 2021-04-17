int f(int a, int b) {
    return a + b;
}

int g(int a, int b);

int main() {
    int a = 10;
    int b = 20;
    f(a, b);
    test: do {
        
    } while(a--);
    goto test;
    
}

int g(int a, int b){
    int c = a - b;
    goto test;
}
