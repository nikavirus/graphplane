#ifndef SHAREDDATA_H
#define SHAREDDATA_H

#include <vector>
#include <QGLWidget>
#include <QGenericMatrix>


// Класс, указатель на который можно будет передавать потокам
class SharedData
{
public:
    SharedData();

    int NAPRX;

    std::vector<std::array<GLfloat, 3>> dots;
    std::vector<std::vector<GLuint>> indexs2;

    std::vector<std::array<GLfloat, 3>> dotsApprx;
    std::vector<std::vector<GLuint>> indexsApprx;

    int lenx, leny, nx, ny;

    double ax, bx, ay, by;
    double dx, dy, ddx, ddy;
    double max_show_func;
    int p=0;

    int gt;

    double (*current_f) (const double&, const double&);
    double (*bessel_func) (SharedData *sd, int i, int j, double x, double y) ;

    std::vector<std::vector<double>> F;
    std::vector<std::vector<double>> Fx;
    std::vector<std::vector<double>> Fy;
    std::vector<std::vector<double>> Fxy;

    std::vector<std::vector<QGenericMatrix<4, 4, double>>> Fij;
    QGenericMatrix<4, 4, double> Ax;
    QGenericMatrix<4, 4, double> AyT;
    std::vector<std::vector<QGenericMatrix<4, 4, double>>> Gammaij;

    int recomended_worker_count=0;

};

#endif // SHAREDDATA_H
