#ifndef MYRENDERWORKER_H
#define MYRENDERWORKER_H

#include <QThread>
#include <QGLWidget>
#include <QMutex>
#include <QWaitCondition>
#include "shareddata.h"

class MyRenderWorker : public QThread
{
    Q_OBJECT
public:
    static const int MAKE_F = 0;
    static const int MAKE_APPRX = 1;
    int make_what;

    SharedData *sd;
    double (*current_f) (const double&, const double&);

    int thread_number, thread_count;
    float max_func, min_func;

//    QMutex waitForGamma;

    MyRenderWorker();
    MyRenderWorker(const MyRenderWorker &);
    void prepare_shared_memory();

    void run() override;

signals:
    void calculation_F_finished();
    void calculating_done();

public slots:
    void terminate_self();
    void start_self();
};

#endif // TESTWORKER_H
