#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardPaths>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QTextStream>
#include <numeric>
#include <QStorageInfo>

#include "crc32.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{

    enum ScanType {
        DirectoryScan,
        GenericScan
    };

    Q_OBJECT

public:
    QDir selectedDir;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnSelectDirectory_clicked();

    void on_chkScanFiles_toggled(bool checked);

    void on_chkRefresh_toggled(bool checked);

    void on_chkCalcCRC_toggled(bool checked);

    void on_chk50_toggled(bool checked);

    void on_chk100_toggled(bool checked);

    void on_chk200_toggled(bool checked);

    void on_chk500_toggled(bool checked);

    void on_chk1000_toggled(bool checked);

    void on_chk2000_toggled(bool checked);

    void on_chk5000_toggled(bool checked);

    void on_chk10000_toggled(bool checked);

    void on_chkEnableDirectoryCreation_toggled(bool checked);

    void createFiles(QDir rootDir, int numFiles, bool enableDirectoryCreation = false);

    void scan(const QDir &directory, QVector<int> numFilesScans, bool enableStorageRefresh = false, bool enableCrc = false);

    void on_btnStartStop_clicked();

    void on_chkVerboseLogging_toggled(bool checked);

private:
    bool running = false;
    int numDirFiles = 0;
    uint secondsPassed = 0;
    QTimer *updateTimer;
    Crc32 *crc32;

    ScanType scanType = GenericScan;
    Ui::MainWindow *ui;

    double probDirCreated = 0.05;
    double probGoBack = 0.15;

    bool scanFiles = false,
        refreshOnScan = false,
        calcCRC = false,
        enableDirectoryCreation = false;

    bool scan50 = false,
        scan100 = false,
        scan200 = false,
        scan500 = false,
        scan1000 = false,
        scan2000 = false,
        scan5000 = false,
        scan10000 = false;

    bool verboseLogging = false;

    void dispatchToMainThread(std::function<void()> callback);
    void startRunTimer();
    void stopRunTimer();
    void analyzeDir();
    void runScan(QDir root, int numFiles);

};
#endif // MAINWINDOW_H
