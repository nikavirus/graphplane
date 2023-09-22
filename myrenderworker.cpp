#include "myrenderworker.h"
#include <QDebug>
#include <qmath.h>

MyRenderWorker::MyRenderWorker()
{
    setTerminationEnabled(true);
}

MyRenderWorker::MyRenderWorker(const MyRenderWorker &)
{
    // Вроде нечего делать
}

void MyRenderWorker::run() {
    std::vector<std::array<GLfloat, 3>> &dots = sd->dots;
    std::vector<std::vector<GLuint>> &indexs2 = sd->indexs2;
    double &ax = sd->ax,  &ay = sd->ay;
    double &dx = sd->dx, &dy=sd->dy, &ddx = sd->ddx, &ddy = sd->ddy;
    int &lenx = sd->lenx, &leny=sd->leny;
    current_f = sd->current_f;
    int &nx = sd->nx, &ny = sd->ny;

    int &gt = sd->gt;

    int i, j;

    if(gt == 0) {

        sd->max_show_func = qAbs(current_f(ax, ay));

        min_func = current_f(ax, ay);
        max_func = current_f(ax, ay);

        // Вычисляем саму функцию во всех точках
        for(i=thread_number; (i<lenx) && (!this->isInterruptionRequested()) ; i=i+thread_count) {
            for(j=0; (j<leny) && (!this->isInterruptionRequested()); ++j) {
                dots[leny*j + i][0] = ax + i*ddx;
                dots[leny*j + i][1] = ay + j*ddy;
                dots[leny*j + i][2] = current_f( ax + i*ddx, ay + j*ddy);
                sd->max_show_func = qMax(sd->max_show_func, qAbs(current_f(ax, ay)));

                min_func = qMin(min_func, dots[leny*j + i][2]);
                max_func = qMax(min_func, dots[leny*j + i][2]);
            }
        }

        if(thread_number == 0) {
            dots[leny * (leny/2) + (lenx/2)][2] += sd->p * 0.01 * sd->max_show_func;
        }

        if(gt == 0) {
            //// Начинаем расписывать индексы для функции
            int index_point;
            for(i=thread_number; (i<leny-1)&& (!this->isInterruptionRequested()); i=i+thread_count) {
                indexs2[i].clear();
                indexs2[i].resize((lenx-1) * 6);

                index_point = 0;
                for(j=0; (j< lenx-1) && (!this->isInterruptionRequested()); ++j){
                    indexs2[i][index_point++] = (i*leny + j);
                    indexs2[i][index_point++] = ((i+1) * leny + j);
                    indexs2[i][index_point++] = ((i+1) * leny + (j+1));
                    indexs2[i][index_point++] = ((i)*leny + j);
                    indexs2[i][index_point++] = ((i) * leny + j+1);
                    indexs2[i][index_point++] = ((i+1) * leny + j+1);
                }
            }
        }

        if(this->isInterruptionRequested()){
            return;
        }
        emit calculating_done();
        return;
    }

    if(gt == 1 || gt == 2) {
        //        qDebug() << "Вычисляем приближение функции";

        if(make_what == MAKE_F){
            for(i=thread_number; (i<nx) && (!this->isInterruptionRequested()); i=i+thread_count) {
                for(j=0; j<ny; ++j) {
                    sd->F[i][j] = current_f(ax + i*dx, ay + j*dy);
                }
            }

            if(thread_number == 0) {
                sd->F[nx/2][ny/2] += sd->p * 0.01 * sd->max_show_func;
            }

            if(this->isInterruptionRequested()) {
                return;
            }
            emit calculation_F_finished();
            return;
        }


        if(make_what == MAKE_APPRX){


            if(gt == 1){
                sd->max_show_func = qAbs(sd->bessel_func(sd, 0, 0,  ax, ay));

                for(i=thread_number; (i<lenx) && (!this->isInterruptionRequested()) ; i=i+thread_count) {
                    for(j=0; (j<leny) && (!this->isInterruptionRequested()); ++j) {
                        sd->dotsApprx[leny*j + i][0] = ax + i*ddx;
                        sd->dotsApprx[leny*j + i][1] = ay + j*ddy;
                        sd->dotsApprx[leny*j + i][2] = sd->bessel_func(sd, i / (sd->NAPRX - 1), j/ (sd->NAPRX - 1),  ax + i*ddx, ay + j*ddy);
                        sd->max_show_func = qMax(sd->max_show_func, (double) qAbs(sd->dotsApprx[leny*j + i][2]));

                    }
                }
            } else {
                sd->max_show_func = qAbs(sd->bessel_func(sd, 0, 0,  ax, ay) - sd->current_f(ax, ay));

                for(i=thread_number; (i<lenx) && (!this->isInterruptionRequested()) ; i=i+thread_count) {
                    for(j=0; (j<leny) && (!this->isInterruptionRequested()); ++j) {
                        sd->dotsApprx[leny*j + i][0] = ax + i*ddx;
                        sd->dotsApprx[leny*j + i][1] = ay + j*ddy;
                        sd->dotsApprx[leny*j + i][2] =
                                qAbs(sd->bessel_func(sd, i / (sd->NAPRX - 1), j/ (sd->NAPRX - 1),  ax + i*ddx, ay + j*ddy) - sd->current_f(ax + i*ddx, ay + j*ddy) );
                        sd->max_show_func = qMax(sd->max_show_func, (double) qAbs(sd->dotsApprx[leny*j + i][2]));

                    }
                }
            }

            int index_point;
            for(i=thread_number; (i<leny-1)&& (!this->isInterruptionRequested()); i=i+thread_count) {
                sd->indexsApprx[i].clear();
                sd->indexsApprx[i].resize((lenx-1) * 6);

                index_point = 0;
                for(j=0; (j< lenx-1) && (!this->isInterruptionRequested()); ++j){
                    sd->indexsApprx[i][index_point++] = (i*leny + j);
                    sd->indexsApprx[i][index_point++] = ((i+1) * leny + j);
                    sd->indexsApprx[i][index_point++] = ((i+1) * leny + (j+1));
                    sd->indexsApprx[i][index_point++] = ((i)*leny + j);
                    sd->indexsApprx[i][index_point++] = ((i) * leny + j+1);
                    sd->indexsApprx[i][index_point++] = ((i+1) * leny + j+1);
                }
            }

            if(this->isInterruptionRequested()){
                return;
            }
            emit calculating_done();
            return;
        }
    }



}

void MyRenderWorker::terminate_self() {
    terminate();
}

void MyRenderWorker::start_self() {
    qDebug() << "Пытаемся запустить себя" ;
    start();
}
