#include <iostream>
#include <string>

using namespace std;

void mostrar(string s, int i) {
    if (i < s.length()) {
        cout << s[i] << " ";
        mostrar(s, i + 1);
    }
}

bool esPalindromo(string s, int i, int f) {
    if (i >= f) return true;
    if (s[i] != s[f]) return false;
    return esPalindromo(s, i + 1, f - 1);
}

int main() {
    string palabra;
    cout << "Ingrese palabra: ";
    cin >> palabra;

    mostrar(palabra, 0);
    cout << endl;

    if (esPalindromo(palabra, 0, palabra.length() - 1)) {
        cout << "Es palindroma" << endl;
    } else {
        cout << "No es palindroma!AAAAA" << endl;
    }

    return 0;
}