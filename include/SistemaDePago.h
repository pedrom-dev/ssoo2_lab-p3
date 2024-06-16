#ifndef SISTEMA_DE_PAGO_H
#define SISTEMA_DE_PAGO_H

class SistemaDePago {
public:
    // Método para obtener la instancia del Singleton
    static SistemaDePago* getInstance();

    // Eliminamos el constructor de copia y el operador de asignación
    SistemaDePago(const SistemaDePago&) = delete;
    void operator=(const SistemaDePago&) = delete;
    int procesarPago();

private:
    // Constructor privado
    SistemaDePago() {}

    // Instancia estática del Singleton
    static SistemaDePago* instance;
};

#endif // SISTEMA_DE_PAGO_H