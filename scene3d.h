#ifndef SCENE3D_H
#define SCENE3D_H

#include <QGLWidget>
#include <vector>
#include <QMutex>
#include <QWaitCondition>
#include <besselcomp.h>
#include "myrenderworker.h"
#include <QMessageBox>

class Scene3D : public QGLWidget
{
    Q_OBJECT
private:
    GLfloat xRot;
    GLfloat yRot;
    GLfloat zRot;
    GLfloat xTra, yTra, zTra;
    GLfloat nSca, xySca;
    GLfloat D;
    GLfloat zSca=1.0;

    QPoint ptrMousePosition;

    const QString appname = "Graph 3D";
    const QString appname_in_calculation = "Подождите, идет вычисление...";
    const QString help_text = QString("<h1>Небольшая справка: </h1> <br />") +
            QString("<b>WASD</b>\t управление сдвигом графика <br />") +
            QString("<b>Стрелочки</b> управление поворотом графика <br />") +
            QString("<b>+ -</b> управление масштабом графика <br />") +
            QString("<h5>Остальные клавиши описаны в требованиях к программе </h5><br />") +
            QString("<b>1</b> изменение состава отображаемых графиков <br />") +
            QString("<b>2 3</b> сжатие/расстяжение XY <br />") +
            QString("<b>4 5</b> изменение nx, ny< br/>") +
            QString("<b>6 7</b> изменение p < br/>") +
            QString("<b>8 9</b> вращение вокруг Oz<br/>") +
            QString("<b>0</b> смена функции<br/>") +
            QString("<h4 style = 'color:red'>Для повторного открытия справки нажмите F1</h4>") +
            QString("<h5>Для закрытия справки нажмите Esc или OK</h5>");

    bool isFirstRender = true;


    int &gt; // тип графика - graph type

    int &nx, &ny, k, p;
    int start_nx, start_ny;

    // область приближения
    double &ax, bx, &ay, by;

    double &dx, &dy, &ddx, &ddy;

    int &lenx, &leny;
    int n_scale;

    QString func_name, scale_info, rotate_info, gt_info, max_info, p_info;

    // Некоторые функции в конструкторе запускают рендеринг. Чтобы он не запустился раньше времени, нужна эта переменная
    bool hasInitialized = false;

    double (*current_f) (const double&, const double&);


    int z_rotate;

    std::vector<std::array<GLfloat, 3>> &dots;
    std::vector<std::vector<GLuint>> &indexs2;

    std::vector<std::array<GLfloat, 3>> &dotsApprx;
    std::vector<std::vector<GLuint>> &indexsApprx;

    std::vector<std::vector<double>> &F;



    void scale_plus();
    void scale_minus();
    void rotate_up();
    void rotate_down();
    void rotate_left();
    void rotate_right();
    void translate_down();
    void translate_up();;
    void defaultScene();

    void translate_left();
    void translate_right();
    void scalexy_up();
    void scalexy_down();
    void change_gt();

    void drawAxis();

    void getVertexArray();
    void getColorArray();
    void getIndexArray();
    void drawFigure();

    void make_dots();
    void set_func();
    void n_plus();
    void n_minus();
    void p_plus();
    void p_minus();
    void make_p_info();
    void rot_z_plus();
    void rot_z_minus();

    void make_label();
    void render_dots();




    void make_rotate_info();
    void make_max_info();


    //Вектор вычислительных потоков
    std::vector<MyRenderWorker> threads;
    int recomended_worker_count = 0;
    int workers_count;
    int workers_in_progress;
    // Иничиализирует потоки, задает общее число потоков
    void make_threads();
    // Прерывает старые потоки и перезапускает их
    void thread_calc();

    //Останавливаем потоки
    void terminate_threads();

    // Устанавливает ссылки на одноименные параметры в классе
    void prepare_shared_memory();

//    QMutex mutex;
//    QWaitCondition waitForGamma;

    BesselComp bc;
    void bessel_comp_init();
    void set_make_what(int);


    void parse_command_line();

    QMessageBox *qmb;

    void zSca_plus();
    void zSca_minus();
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    /*virtual*/ void mousePressEvent(QMouseEvent* pe);   // методы обработки события мыши при нажатии клавиши мыши
    /*virtual*/ void mouseMoveEvent(QMouseEvent* pe);    // методы обработки события мыши при перемещении мыши
//    /*virtual*/ void mouseReleaseEvent(QMouseEvent* pe); // методы обработки событий мыши при отжатии клавиши мыши
    /*virtual*/ void wheelEvent(QWheelEvent* pe);        // метод обработки событий колесика мыши
    /*virtual*/ void keyPressEvent(QKeyEvent* pe);       // методы обработки события при нажатии определенной клавиши
public:

    Scene3D(QWidget* parent = 0);

public slots:

    void calculating_F_finished();
    void calculation_Gamma_finished();

    // Получает сигнал от потоков о завершении вычисления
    void threads_calc_finished();

signals:

};


#endif // GRAPH3D_H
