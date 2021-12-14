/**
 * This class represents complex numbers with a real and imaginary component.
 */
class Complex {
	public: 
		float real;
		float imaginary;
		bool lessThan(float num) {
			return (dist(real, imaginary, 0, 0) < num);
		}
		Complex multiply(Complex x) {
			float newR = (this->real*x.real) - this->imaginary*x.imaginary;
			float newI = (this->real*x.imaginary) + this->imaginary*x.real;
			Complex ret(newR, newI);
			return ret;
		}
		Complex add(Complex x) {
			float newR = this->real + x.real;
			float newI = this->imaginary + x.imaginary;
			Complex ret(newR, newI);
			return ret;
		}
		Complex(float real, float imaginary) {
			this->real = real;
			this->imaginary = imaginary;
		}
		float dist(float x1, float y1, float x2, float y2) {
			float result = sqrt(pow(x2-x1, 2) + pow(y2-y1, 2));
		return result;
}
};