#include <QtGui>
#include <math.h>

#include "scene3d.h"
#include <qmath.h>
#include "shareddata.h"
#include <QApplication>

const static int NAPRX=4;


static double f_0(const double &, const double &) { return 1; }
static double f_1(const double &x, const double &) { return x; }
static double f_2(const double &, const double &y) { return y; }
static double f_3(const double &x, const double &y) { return x+y; }
static double f_4(const double &x, const double &y) { return qSqrt(x*x + y*y); }
static double f_5(const double &x, const double &y) { return x*x + y*y; }
static double f_6(const double &x, const double &y) { return qExp(x*x - y*y); }
static double f_7(const double &x, const double &y) { return 1/(25*(x*x + y*y) + 1); }

SharedData shared_data;


Scene3D::Scene3D(QWidget* parent) : QGLWidget(parent), gt(shared_data.gt), nx(shared_data.nx), ny(shared_data.ny),
    ax(shared_data.ax), ay(shared_data.ay), dx(shared_data.dx), dy(shared_data.dy),
    ddx(shared_data.ddx), ddy(shared_data.ddy) , lenx(shared_data.lenx),  leny(shared_data.leny), dots(shared_data.dots),
    indexs2(shared_data.indexs2), dotsApprx(shared_data.dotsApprx), indexsApprx(shared_data.indexsApprx), F(shared_data.F)  {

    parse_command_line();
    make_threads();
    bessel_comp_init();

    xRot = -90; yRot=0; zRot=0; xTra = 0; yTra = 0; zTra=0; nSca=1, xySca=1, zSca = 1;

     D = qSqrt((ax-bx)*(ax-bx) + (ay-by)*(ay-by));

    shared_data.NAPRX = NAPRX;


    gt = -1;
    change_gt();
    set_func();

    shared_data.bessel_func = bessel_func;

    n_scale = 0;
    scale_info.clear();
    QTextStream(&scale_info) << "nx, ny = " << nx << ", " << ny;

    z_rotate = 0;


    setWindowTitle(appname);

    hasInitialized = true;
    render_dots();

    qmb = new QMessageBox(this);
    qmb->setWindowTitle("Справка");
    qmb->setModal(false);
    qmb->setText(help_text);
    qmb->hide();

}

void Scene3D::calculating_F_finished()
{

    // Подготавливаемся к запуску вычисления коэффицентов Бесселя
    workers_in_progress--;

    if(workers_in_progress == 0) {
        bc.start_computing();
    }
}

void Scene3D::calculation_Gamma_finished()
{
    // Закончили вычислять коэффицент гамма

    set_make_what(MyRenderWorker::MAKE_APPRX);
    thread_calc();
}

void Scene3D::threads_calc_finished()
{
    //TODO Уменьшает workers_in_progress
    // Перерисовка экрана
    // Меняем заголовок

    workers_in_progress--;

    if(workers_in_progress == 0) {
        setWindowTitle(appname);
//        make_max_info();


        updateGL();
    }
}

void Scene3D::set_func() {
    func_name = QString("k=") + QString::number(k) + QString(", ");
    switch(k) {
    case 0:
        current_f=f_0;
        func_name += QString("f(x,y)=1");
        break;
    case 1:
        current_f=f_1;
        func_name+=QString("f(x,y)=x");
        break;
    case 2:
        current_f=f_2;
        func_name += QString( "f(x,y)=y");
        break;
    case 3:
        current_f=f_3;
        func_name +=  QString("f(x,y)=x+y");
        break;
    case 4:
        current_f=f_4;
        func_name += QString("f(x,y)=sqrt(x*x + y*y)");
        break;
    case 5:
        current_f=f_5;
        func_name += QString("f(x,y)=x*x + y*y");
        break;
    case 6:
        current_f=f_6;
        func_name += QString( "f(x,y)=Exp(x*x - y*y)");
        break;
    case 7:
        current_f=f_7;
        func_name += QString( "f(x,y)=1/(25*(x*x + y*y) + 1)");
        break;
    }


    // Объявляем всем потокам, что функция поменялась
    shared_data.current_f = current_f;

    render_dots();
}

void Scene3D::initializeGL() {
    qglClearColor(Qt::gray);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    glEnable(GL_CULL_FACE);

    glEnableClientState(GL_VERTEX_ARRAY);
    //    glEnableClientState(GL_COLOR_ARRAY);


}

void Scene3D::resizeGL(int w, int h) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLfloat ratio = (GLfloat) h/(GLfloat) w;


    if(w<h)
        glOrtho(-1.0/ratio, 1.0/ratio, -1.0, 1.0, -D, D);
    else
        glOrtho(-1.0, 1.0, -1.0*ratio, 1.0*ratio, -D, D);


    // Поле просмотра
    glViewport(0, 0, (GLint)w, (GLint)h);
}

/*virtual*/ void Scene3D::paintGL() // рисование
{
    // glClear(GL_COLOR_BUFFER_BIT); // окно виджета очищается текущим цветом очистки
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // очистка буфера изображения и глубины

    glMatrixMode(GL_MODELVIEW); // устанавливает положение и ориентацию матрице моделирования
    glLoadIdentity();           // загружает единичную матрицу моделирования


    // последовательные преобразования
    glScalef(2*nSca/D * xySca, 2*nSca/D * xySca, zSca);        // масштабирование
    glTranslatef(xTra, zTra/* - shared_data.max_show_func/2*/, 0.0f);    // трансляция
    glRotatef(xRot, 1.0f, 0.0f, 0.0f); // поворот вокруг оси X
    glRotatef(yRot, 0.0f, 1.0f, 0.0f); // поворот вокруг оси Y
    glRotatef(zRot, 0.0f, 0.0f, 1.0f); // поворот вокруг оси Z


    drawAxis();   // рисование осей координат
    drawFigure(); // нарисовать фигуру

    if(isFirstRender) {
        isFirstRender = false;
        qmb->show();
    }


}

/*virtual*/void Scene3D::mousePressEvent(QMouseEvent* pe) // нажатие клавиши мыши
{
    // при нажатии пользователем кнопки мыши переменной ptrMousePosition будет
    // присвоена координата указателя мыши
    ptrMousePosition = pe->pos();

    // ptrMousePosition = (*pe).pos(); // можно и так написать
}

///*virtual*/void Scene3D::mouseReleaseEvent(QMouseEvent* pe) // отжатие клавиши мыши
//{
//    // некоторые функции, которые должны выполняться при отжатии клавиши мыши
//}

/*virtual*/void Scene3D::mouseMoveEvent(QMouseEvent* pe) // изменение положения стрелки мыши
{
    xRot += 180/nSca*(GLfloat)(pe->y()-ptrMousePosition.y())/height(); // вычисление углов поворота
    zRot += 180/nSca*(GLfloat)(pe->x()-ptrMousePosition.x())/width();

    ptrMousePosition = pe->pos();

    updateGL(); // обновление изображения
}

/*virtual*/void Scene3D::wheelEvent(QWheelEvent* pe) // вращение колёсика мыши
{
    if ((pe->delta())>0) scale_plus(); else if ((pe->delta())<0) scale_minus();

    updateGL(); // обновление изображения
}

/*virtual*/void Scene3D::keyPressEvent(QKeyEvent* pe) // нажатие определенной клавиши
{
    switch (pe->key())
    {
    case Qt::Key_Plus:
        scale_plus();     // приблизить сцену
        break;

    case Qt::Key_Equal:
        scale_plus();     // приблизить сцену
        break;

    case Qt::Key_Minus:
        scale_minus();    // удалиться от сцены
        break;

    case Qt::Key_Up:
        rotate_up();      // повернуть сцену вверх
        break;

    case Qt::Key_Down:
        rotate_down();    // повернуть сцену вниз
        break;

    case Qt::Key_Left:
        rotate_left();     // повернуть сцену влево
        break;

    case Qt::Key_Right:
        rotate_right();   // повернуть сцену вправо
        break;

    case Qt::Key_S:
        translate_down(); // транслировать сцену вниз
        break;

    case Qt::Key_W:
        translate_up();   // транслировать сцену вверх
        break;

    case Qt::Key_Space:  // клавиша пробела
        defaultScene();   // возвращение значений по умолчанию
        break;

    case Qt::Key_Escape: // клавиша "эскейп"
        this->close();    // завершает приложение
        break;

    case Qt::Key_0:       //Меняем отображаемую функцию
        k = (k+1) % 8;
        set_func();
        break;
    case Qt::Key_1:
        change_gt();
        break;
    case Qt::Key_4:         //Увеличиваем число точек апроксимации
        n_plus();
        break;
    case Qt::Key_5:         //Уменьшаем число точек апроксимации
        n_minus();
        break;
    case Qt::Key_8:
        rot_z_plus();
        break;
    case Qt::Key_9:
        rot_z_minus();
        break;
    case Qt::Key_A:
        translate_left();
        break;
    case Qt::Key_D:
        translate_right();
        break;
    case Qt::Key_2:
        scalexy_up();
        break;

    case Qt::Key_3:
        scalexy_down();
        break;

    case Qt::Key_6:
        p_plus();
        break;
    case Qt::Key_7:
        p_minus();
        break;
    case Qt::Key_F1:
        if(qmb->isHidden()){
            qmb->show();
        } else {
            qmb->hide();
        }
        break;
    case Qt::Key_O:
        zSca_plus();
        break;
    case Qt::Key_P:
        zSca_minus();
        break;

    }

    updateGL(); // обновление изображения
}

void Scene3D::make_rotate_info() {
    rotate_info= QString("OZ поворот: ") +  QString::number(zRot) + QString("°") ;
}

void Scene3D::make_max_info()
{
    max_info = "max|f| = " + QString::number(shared_data.max_show_func);
}

void Scene3D::make_threads()
{
    unsigned long i;
    //TODO Иничиализирует данные в потоках, задает общее число потоков
    if(recomended_worker_count == 0)
        workers_count = QThread::idealThreadCount();
    else
        workers_count = recomended_worker_count;

    threads.resize(workers_count);
    for(i=0; i<threads.size(); ++i) {
        threads[i].thread_count = workers_count;
        threads[i].thread_number = i;
        threads[i].sd = &shared_data;
        QObject::connect(&threads[i], SIGNAL(calculating_done()), this , SLOT(threads_calc_finished()));
        QObject::connect(&threads[i], SIGNAL(calculation_F_finished()), this , SLOT(calculating_F_finished()));
    }

    workers_in_progress = 0;

}

void Scene3D::thread_calc()
{
    //TODO Останавливает работу потоков
    // Запускает вычисления потоков
    // Меняет переменную workers_in_progress

    unsigned long i;

    workers_in_progress = workers_count;
    setWindowTitle(appname_in_calculation);
    for(i=0; i<threads.size(); ++i)  {
        threads[i].start();
    }
}

void Scene3D::terminate_threads()
{
    unsigned long i;
    for(i=0; i<threads.size(); ++i) {
        if(threads[i].isRunning()) {
            threads[i].requestInterruption();
            threads[i].wait();
        }
    }

    bc.terminate();


    workers_in_progress = 0;
}

void Scene3D::bessel_comp_init()
{
    bc.sd = &shared_data;
    bc.recommended_worker_count = recomended_worker_count;
    QObject::connect(&bc, SIGNAL(computation_complete()), this , SLOT(calculation_Gamma_finished()));
}

void Scene3D::set_make_what(int what)
{
    for(unsigned long i=0; i<threads.size(); ++i) {
        threads[i].make_what = what;
    }
}

void Scene3D::parse_command_line()
{
    ax = -10; bx = 10; ay = -10; by = 10;
    start_nx = nx = 5;
    start_ny = ny = 5;
    k=0;


    if(QApplication::arguments().size() < 1) {
        qDebug() << "Аргументы коммандной строки не заданы, используем стандартные параметры";
    }


    if(QApplication::arguments().size() >= 4) {
        qDebug() << "nx: " << QApplication::arguments().at(2).toInt();
        qDebug() << "ny: " << QApplication::arguments().at(3).toInt();
        if((QApplication::arguments().at(2).toInt() >=3) && (QApplication::arguments().at(3).toInt() >=3)) {
            start_nx = nx = QApplication::arguments().at(2).toInt();
            start_ny = ny = QApplication::arguments().at(3).toInt();
        }
    } else if(QApplication::arguments().size() < 4){
        qDebug() << "Параметры nx, ny не заданы, используем стандартные параметры";
    }
    if(QApplication::arguments().size() >= 5) {
        qDebug() << "k = " << QApplication::arguments().at(4).toInt();
        if((QApplication::arguments().at(4).toInt() >=0) && (QApplication::arguments().at(4).toInt() <=7)) {
            k=QApplication::arguments().at(4).toInt();
        }
    } else if(QApplication::arguments().size() < 5){
        qDebug() << "Параметр k не задан, используем стандартное значение k=0";
    }
    if(QApplication::arguments().size() >= 7) {
        qDebug() << "p = " << QApplication::arguments().at(6).toInt();
        if( QApplication::arguments().at(6).toInt() > 0) {
            recomended_worker_count =  QApplication::arguments().at(6).toInt();
            shared_data.recomended_worker_count =  QApplication::arguments().at(6).toInt();
        }
    } else if(QApplication::arguments().size() < 7){
        qDebug() << "Параметр p не задан, используем стандартные значения";
    }



}

void Scene3D::zSca_plus()
{
    zSca*=1.1f;
}

void Scene3D::zSca_minus()
{
    zSca/=1.1f;
}


void Scene3D::rot_z_plus() {
    zRot += 15;
}

void Scene3D::rot_z_minus() {
    zRot -= 15;
}

void Scene3D::n_plus() {
    n_scale++;
    nx = start_nx * qPow(2, n_scale);
    ny = start_ny * qPow(2, n_scale);
    scale_info.clear();
    QTextStream(&scale_info) << "nx, ny = " << nx << ", " << ny;
    render_dots();
}
void Scene3D::n_minus() {
    n_scale--;
    nx = start_nx * qPow(2, n_scale);
    ny = start_ny * qPow(2, n_scale);
    if(nx < 3 || ny < 3) {
        n_plus();
        return;
    }
    scale_info.clear();
    QTextStream(&scale_info) << "nx, ny = " << nx << ", " << ny;
    render_dots();
}

void Scene3D::p_plus()
{
    shared_data.p += 1;
    render_dots();
}

void Scene3D::p_minus()
{
//    qDebug() << "Проходит сигнал на минус";
    if(shared_data.p != 0) shared_data.p --;
    render_dots();
}

void Scene3D::make_p_info()
{
    p_info = "p=" + QString::number(shared_data.p) + " s= " + QString::number(xySca);
}

void Scene3D::scale_plus() // приблизить сцену
{
    nSca = nSca*1.1;
}

void Scene3D::scale_minus() // удалиться от сцены
{
    nSca = nSca/1.1;
}

void Scene3D::rotate_up() // повернуть сцену вверх
{
    xRot += 1.0;
}

void Scene3D::rotate_down() // повернуть сцену вниз
{
    xRot -= 1.0;
}

void Scene3D::rotate_left() // повернуть сцену влево
{
    zRot += 1.0;
}

void Scene3D::rotate_right() // повернуть сцену вправо
{
    zRot -= 1.0;
}

void Scene3D::translate_down() // транслировать сцену вниз
{
    zTra -= 0.25;
}

void Scene3D::translate_up() // транслировать сцену вверх
{
    zTra += 0.25;
}

void Scene3D::defaultScene() // наблюдение сцены по умолчанию
{
    xRot=-90; yRot=0; zRot=0; zTra=0; nSca=1;
}

void Scene3D::translate_left()
{
    xTra -= 0.5;
}

void Scene3D::translate_right()
{
    xTra += 0.5;
}

void Scene3D::scalexy_up()
{
    xySca *= 2;
}

void Scene3D::scalexy_down()
{
    xySca /=2;
}

void Scene3D::change_gt()
{
    gt = (gt+1) % 3;
    switch(gt) {
    case 0:
        gt_info = "График функции";
        break;
    case 1:
        gt_info = "График приближения функции";
        break;
    case 2:
        gt_info = "График погрешности приближения функции";
        break;
    }

    render_dots();
}


void Scene3D::drawAxis() // построить оси координат
{
    glLineWidth(3.0f); // устанавливаю ширину линии приближённо в пикселях
    // до вызова команды ширина равна 1 пикселю по умолчанию

    glColor4f(1.00f, 0.00f, 0.00f, 1.0f); // устанавливается цвет последующих примитивов
    // ось x красного цвета
    glBegin(GL_LINES); // построение линии
    glVertex3f( bx,  0.0f,  0.0f); // первая точка
    glVertex3f( 0.0f,  0.0f,  0.0f); // вторая точка
    glEnd();

    QColor halfGreen(0, 128, 0, 255);
    qglColor(halfGreen);
    glBegin(GL_LINES);
    // ось y зеленого цвета
    glVertex3f( 0.0f,  by,  0.0f);
    glVertex3f( 0.0f,  0.0f,  0.0f);

    glColor4f(0.00f, 0.00f, 1.00f, 1.0f);
    // ось z синего цвета
    glVertex3f( 0.0f,  0.0f,  shared_data.max_show_func);
    glVertex3f( 0.0f,  0.0f,  0);
    glEnd();
}


void Scene3D::render_dots() { //Определить массивы точек, по которым будем рисовать

    int i;
    if(hasInitialized == false) return;

    // Останавливаем все потоки
    terminate_threads();

    lenx = (nx-1)*(NAPRX - 1) + 1;
    leny = (ny-1)*(NAPRX - 1) + 1;
    dots.resize(lenx * leny);

    dx = (bx-ax)/(nx-1);
    dy = (by-ay)/(ny-1);

    ddx = dx/(NAPRX - 1);
    ddy = dy/(NAPRX - 1);

    if((gt == 0) || (gt ==2)){
        indexs2.clear();
        indexs2.resize(leny-1);
    }

    if((gt == 1) || (gt == 2)){

        F.resize(nx);
        for( i=0; i<nx; ++i)
            F[i].resize(ny);

        shared_data.Fx.resize(nx);
        shared_data.Fy.resize(nx);
        shared_data.Fxy.resize(nx);

        shared_data.Fij.resize(nx-1);
        shared_data.Gammaij.resize(nx-1);

        for(i=0; i<nx; ++i) {
            shared_data.Fx[i].resize(ny);
            shared_data.Fy[i].resize(ny);
            shared_data.Fxy[i].resize(ny);
        }
        for(i=0; i<nx-1; ++i) {
            shared_data.Fij[i].resize(ny-1);
            shared_data.Gammaij[i].resize(ny-1);
        }

        dotsApprx.resize(lenx * leny);


        indexsApprx.clear();
        indexsApprx.resize(leny-1);

        set_make_what(MyRenderWorker::MAKE_F);
    }

    thread_calc();
}

void Scene3D::drawFigure() // построить фигуру
{
    int i;

    glLineWidth(1);

    glColor4f(0.00f, 0.00f, 1.00f, 1.0f);

    if(workers_in_progress == 0){

        if(gt == 0){
            // Если необходимо нарисовать график функции, рисуем его
            glVertexPointer(3, GL_FLOAT, 0, dots.data());
            for(i=0; i < (leny-1); ++i) {
                glDrawElements(GL_LINE_STRIP, (lenx-1)*6, GL_UNSIGNED_INT, indexs2[i].data());
            }
        }

        if(gt == 1){
            glVertexPointer(3, GL_FLOAT, 0, dotsApprx.data());
            // Если необходимо нарисовать приближение функции, рисуем его
            for(i=0; i < (leny-1); ++i) {
                glDrawElements(GL_LINE_STRIP, (lenx-1)*6, GL_UNSIGNED_INT, indexsApprx[i].data());
            }
        }
        if(gt == 2) {
                glVertexPointer(3, GL_FLOAT, 0, dotsApprx.data());
                // Если необходимо нарисовать приближение функции, рисуем его
                for(i=0; i < (leny-1); ++i) {
                    glDrawElements(GL_LINE_STRIP, (lenx-1)*6, GL_UNSIGNED_INT, indexsApprx[i].data());
                }
        }

    }

    make_label();



}

void Scene3D::make_label() {
    make_p_info();
    make_max_info();
    make_rotate_info();



    glColor3ub(250, 209, 220);
    renderText(10, 20, gt_info);
    renderText(10, 35, func_name);
    renderText(10, 50, scale_info);
    renderText(10, 65, rotate_info);
    renderText(10, 80, max_info);
    renderText(10, 95, p_info);
}


