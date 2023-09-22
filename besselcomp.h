#ifndef BESSELCOMP_H
#define BESSELCOMP_H

#include <QThread>
#include <QGLWidget>
#include <QMutex>
#include <QWaitCondition>
#include "shareddata.h"



// Класс потока, который используется вычислителем коэффицентов Бесселя
class BesselCompThread : public QThread
{
    Q_OBJECT
public:

    static const int MAKE_FXFY  = 0;
    static const int MAKE_FXY   = 1;
    static const int MAKE_Fij   = 2;
    static const int MAKE_GAMMA = 3;
//    static const int MAKE_TMP = 4;

    int make_what;

    SharedData *sd;

    int thread_number, thread_count;

    BesselCompThread();
    BesselCompThread(const BesselCompThread &);


    void run() override;
    void GammaMultiplication(int i, int j);

signals:
    void FxFy_complete();
    void Fxy_complete();
    void Fij_complete();
    void Gamma_complete();
};


class BesselComp: public QObject
{
    Q_OBJECT

public:

    SharedData *sd;
    int recommended_worker_count=0;

    BesselComp();

    void start_computing();

    void terminate();


protected:
    int workers_in_progress = 0;
    std::vector<BesselCompThread> threads;

    int workers_count = 0;

    void make_AxnAy();
    void make_threads();
    void set_make_what(int);
    void run_threads();

signals:
    void computation_complete();

public slots:
    void FxFy_complete();
    void Fxy_complete();
    void Fij_complete();
    void Gamma_complete();
};

double bessel_func(SharedData *sd, int i, int j, double x, double y) ;

#endif // BESSELCOMP_H
