

class MotorController{
    private:
        bool encendido = false;
    public:
        MotorController(int pinMotor);
        void encender(uint16_t RPM, uint16_t timepo);
        void apagar();


}