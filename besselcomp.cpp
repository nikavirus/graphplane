#include "besselcomp.h"
#include <qmath.h>

BesselComp::BesselComp()
{

}

void BesselComp::start_computing()
{
    make_AxnAy();

    terminate();
    make_threads();

    set_make_what(BesselCompThread::MAKE_FXFY);

    // Запускаем наши потоки
    run_threads();
}

void BesselComp::terminate()
{
    unsigned long i;
    for(i=0; i<threads.size(); ++i) {
        if(threads[i].isRunning()) {
            threads[i].requestInterruption();
            threads[i].wait();
        }
    }

}

void BesselComp::make_AxnAy()
{
    sd->Ax(0,0) = 1;                        sd->Ax(0,1) = 0;                        sd->Ax(0,2) = 0;                        sd->Ax(0,3) = 0;
    sd->Ax(1,0) = 0;                        sd->Ax(1,1) = 1;                        sd->Ax(1,2) = 0;                        sd->Ax(1,3) = 0;
    sd->Ax(2,0) = -3/(qPow(sd->dx, 2));      sd->Ax(2,1) = -2/(qPow(sd->dx, 1));    sd->Ax(2,2) =  3/(qPow(sd->dx, 2));     sd->Ax(2,3) = -1/(qPow(sd->dx, 1));
    sd->Ax(3,0) =  2/(qPow(sd->dx, 3));      sd->Ax(3,1) =  1/(qPow(sd->dx, 2));    sd->Ax(3,2) = -2/(qPow(sd->dx, 3));     sd->Ax(3,3) =  1/(qPow(sd->dx, 2));

    sd->AyT(0,0) = 1;                        sd->AyT(1,0) = 0;                        sd->AyT(2,0) = 0;                        sd->AyT(3,0) = 0;
    sd->AyT(0,1) = 0;                        sd->AyT(1,1) = 1;                        sd->AyT(2,1) = 0;                        sd->AyT(3,1) = 0;
    sd->AyT(0,2) = -3/(qPow(sd->dy, 2));      sd->AyT(1,2) = -2/(qPow(sd->dy, 1));    sd->AyT(2,2) =  3/(qPow(sd->dy, 2));     sd->AyT(3,2) = -1/(qPow(sd->dy, 1));
    sd->AyT(0,3) =  2/(qPow(sd->dy, 3));      sd->AyT(1,3) =  1/(qPow(sd->dy, 2));    sd->AyT(2,3) = -2/(qPow(sd->dy, 3));     sd->AyT(3,3) =  1/(qPow(sd->dy, 2));

}

void BesselComp::make_threads()
{


    unsigned long i;
    // Если потоки уже созданы - не трогать их
    if(workers_count != 0) return;

    if(sd->recomended_worker_count == 0)
        workers_count = QThread::idealThreadCount();
    else
        workers_count = recommended_worker_count;

    threads.resize(workers_count);
    for(i=0; i<threads.size(); ++i) {
        threads[i].thread_count = workers_count;
        threads[i].thread_number = i;
        threads[i].sd = sd;
        QObject::connect(&threads[i], SIGNAL(FxFy_complete()), this , SLOT(FxFy_complete()));
        QObject::connect(&threads[i], SIGNAL(Fxy_complete()), this , SLOT(Fxy_complete()));
        QObject::connect(&threads[i], SIGNAL(Fij_complete()), this , SLOT(Fij_complete()));

        QObject::connect(&threads[i], SIGNAL(Gamma_complete()), this , SLOT(Gamma_complete()));

    }

    workers_in_progress = 0;

}

void BesselComp::set_make_what(int what)
{
    for(unsigned long i=0; i<threads.size(); ++i) {
        threads[i].make_what = what;
    }

}

void BesselComp::run_threads()
{
    workers_in_progress = workers_count;
    for(unsigned long i=0; i<threads.size(); ++i)  {
        threads[i].start();
    }
}

void BesselComp::FxFy_complete()
{
    workers_in_progress--;
    if(workers_in_progress == 0) {
        set_make_what(BesselCompThread::MAKE_FXY);
        run_threads();
    }

}

void BesselComp::Fxy_complete()
{

    workers_in_progress--;

    if(workers_in_progress == 0) {
        set_make_what(BesselCompThread::MAKE_Fij);
        run_threads();
    }
}

void BesselComp::Fij_complete()
{
    workers_in_progress--;

    if(workers_in_progress == 0) {
        set_make_what(BesselCompThread::MAKE_GAMMA);
        run_threads();
    }
}

void BesselComp::Gamma_complete()
{
    workers_in_progress--;

    if(workers_in_progress == 0) {
        // Посчитали Gamma
        emit computation_complete();
    }

}

BesselCompThread::BesselCompThread(): QThread()
{
    // Чтобы компилятор не ругался
}

BesselCompThread::BesselCompThread(const BesselCompThread &) : QThread()
{
    // Чтобы компилятор не ругался
}

void BesselCompThread::run()
{
    int i, j;
    //// Вычисляем матрицы Fx и Fy, предполагается, что матрицы нулевые

    int num_of_columns = sd->nx, num_of_rows = sd->ny;

    if(make_what == MAKE_FXFY) {

        for(i=thread_number; (i<num_of_columns) && (!this->isInterruptionRequested()); i=i+thread_count) {
            // Бегаем по столбцам
            for(j=0; (j<num_of_rows) && (!this->isInterruptionRequested()); ++j) {
                // Бегаем по строкам
                if(j==0) {
                    sd->Fx[j][i] = (sd->F[2][i] - sd->F[0][i]) / (2*sd->dx);
                } else if(j== num_of_rows-1) {
                    sd->Fx[j][i] = (sd->F[num_of_rows-1][i] - sd->F[num_of_rows - 3][i]) / (2*sd->dx);
                } else {
                    sd->Fx[j][i] = (sd->F[j+1][i] - sd->F[j-1][i]) / (2*sd->dx);
                }
            }
        }


        // Вычисляем Fy
        for(i=thread_number; (i<num_of_rows) && (!this->isInterruptionRequested()); i=i+thread_count) {
            // Бегаем по строкам
            for(j=0; (j<num_of_columns) && (!this->isInterruptionRequested()); ++j) {
                // Бегаем по столбцам
                if(j==0) {
                    sd->Fy[i][j] = (sd->F[i][2] - sd->F[i][0]) / (2*sd->dy) ;
                } else if(j== num_of_rows-1) {
                    sd->Fy[i][j] = (sd->F[i][num_of_rows-1] - sd->F[i][num_of_rows - 3]) / (2*sd->dy);
                } else {
                    sd->Fy[i][j] = (sd->F[i][j+1] - sd->F[i][j-1]) / (2*sd->dy);
                }
            }
        }

        emit FxFy_complete();

    }



    if(make_what == MAKE_FXY){

        for(i=thread_number; (i<num_of_columns) && (!this->isInterruptionRequested()); i=i+thread_count) {
            // Бегаем по столбцам
            for(j=0; (j<num_of_rows) && (!this->isInterruptionRequested()); ++j) {
                // Бегаем по строкам
                if(j==0) {
                    sd->Fxy[j][i] = (sd->Fy[2][i] - sd->Fy[0][i]) / (2*sd->dx);
                } else if(j== num_of_rows-1) {
                    sd->Fxy[j][i] = (sd->Fy[num_of_rows-1][i] - sd->Fy[num_of_rows - 3][i]) / (2*sd->dx);
                } else {
                    sd->Fxy[j][i] = (sd->Fy[j+1][i] - sd->Fy[j-1][i]) / (2*sd->dx);
                }
            }
        }
        //        }

        emit Fxy_complete();

    }


    if(make_what == MAKE_Fij) {

        for(i = thread_number; (i < num_of_rows - 1) && (!this->isInterruptionRequested()); i = i+thread_count) {
            for(j=0; (j<num_of_columns - 1) && (!this->isInterruptionRequested()); ++j) {
                // Начинаем формировать Fij
                sd->Fij[i][j](0, 0) = sd->F[i][j];  sd->Fij[i][j](0, 1) = sd->Fy[i][j];     sd->Fij[i][j](0, 2) = sd->F[i][j+1];  sd->Fij[i][j](0, 3) = sd->Fy[i][j+1];
                sd->Fij[i][j](1, 0) = sd->Fx[i][j];  sd->Fij[i][j](1, 1) = sd->Fxy[i][j];  sd->Fij[i][j](1, 2) = sd->Fx[i][j+1];  sd->Fij[i][j](1, 3) = sd->Fxy[i][j+1];
                sd->Fij[i][j](2, 0) = sd->F[i+1][j];  sd->Fij[i][j](2, 1) = sd->Fy[i+1][j];  sd->Fij[i][j](2, 2) = sd->F[i+1][j+1];  sd->Fij[i][j](2, 3) = sd->Fy[i+1][j+1];
                sd->Fij[i][j](3, 0) = sd->Fx[i+1][j];  sd->Fij[i][j](3, 1) = sd->Fxy[i+1][j];  sd->Fij[i][j](3, 2) = sd->Fx[i+1][j+1];  sd->Fij[i][j](3, 3) = sd->Fxy[i+1][j+1];
            }
        }

        //        }

        emit Fij_complete();
    }

    if(make_what == MAKE_GAMMA) {

        if(thread_number == 0){
            for(i = 0; (i < num_of_rows - 1) && (!this->isInterruptionRequested()); i = i+1) {
                for(j=0; (j<num_of_columns - 1) && (!this->isInterruptionRequested()); ++j) {
                    GammaMultiplication(i,j);
                }
            }
        }
        emit Gamma_complete();
    }

}

void BesselCompThread::GammaMultiplication(int i, int j)
{
    int k,l,p,q;

    for(k=0; k<4; ++k) {
        for(l=0; l<4; ++l) {

            sd->Gammaij[i][j](k,l) = 0;
            for(p=0; p<4; ++p) {
                for(q=0; q<4; ++q){
                    sd->Gammaij[i][j](k,l) += sd->Ax(k,p) * sd->Fij[i][j](p,q) * sd->AyT(q, l);
                }
            }
        }
    }
}

double bessel_func(SharedData *sd, int i, int j, double x, double y) {

    if(i==(sd->nx-1)) --i;
    if(j==(sd->ny-1)) --j;

    double dxi[4], dx = (x-(sd->ax + i*sd->dx));
    dxi[0] = 1;
    dxi[1] = dxi[0] * dx;
    dxi[2] = dxi[1] * dx;
    dxi[3] = dxi[2] * dx;

    double dyi[4], dy = (y-(sd->ay + j*sd->dy));
    dyi[0] = 1;
    dyi[1] = dyi[0] * dy;
    dyi[2] = dyi[1] * dy;
    dyi[3] = dyi[2] * dy;

    int k, l; double answer = 0;
    for(k=0; k<4; ++k){
        for(l=0; l<4; ++l){
            answer+= sd->Gammaij[i][j](k,l) * dxi[k] * dyi[l];
        }
    }

    return answer;

}
