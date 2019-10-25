#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    crc32 = new Crc32;
    selectedDir.setPath(QApplication::instance()->applicationDirPath());
    ui->lblSelectedDirecvtory->setText(selectedDir.path());
    analyzeDir();
}

MainWindow::~MainWindow()
{
    delete crc32;
    delete ui;
}


void MainWindow::on_btnSelectDirectory_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setFilter(QDir::Filter::Dirs);
    fileDialog->setOption(QFileDialog::Option::ShowDirsOnly);
    fileDialog->setFileMode(QFileDialog::FileMode::DirectoryOnly);
    QDialog::DialogCode localExec = static_cast<QDialog::DialogCode>(fileDialog->exec());
    if(localExec == QDialog::DialogCode::Accepted) {
        selectedDir.setPath(fileDialog->directory().path());
        ui->lblSelectedDirecvtory->setText(selectedDir.path());
    }
    fileDialog->deleteLater();

    analyzeDir();
}


void MainWindow::on_chkScanFiles_toggled(bool checked)
{
    this->scanFiles = checked;
    ui->layoutDirectorySelector->setEnabled(!checked);

    scanType = (checked) ? ScanType::GenericScan : ScanType::DirectoryScan;
    ui->layoutFileNumberOptions_2->setEnabled(checked);

    ui->output->append(QString("Changed Scan Mode to %1 Scan").arg(checked ? "Generic" : "Directory"));
}

void MainWindow::on_chkRefresh_toggled(bool checked)
{
    this->refreshOnScan = checked;
    ui->output->append(QString("Refresh Storage capacity after each file scan: %1").arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chkCalcCRC_toggled(bool checked)
{
    this->calcCRC = checked;
    ui->output->append(QString("Calculate file CRC for each file scanned: %1").arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk50_toggled(bool checked)
{
    scan50 = checked;
    ui->output->append(QString("%1 File scan %2").arg(50).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk100_toggled(bool checked)
{
    scan100 = checked;
    ui->output->append(QString("%1 File scan %2").arg(100).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk200_toggled(bool checked)
{
    scan200 = checked;
    ui->output->append(QString("%1 File scan %2").arg(200).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk500_toggled(bool checked)
{
    scan500 = checked;
    ui->output->append(QString("%1 File scan %2").arg(500).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk1000_toggled(bool checked)
{
    scan1000 = checked;
    ui->output->append(QString("%1 File scan %2").arg(1000).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk2000_toggled(bool checked)
{
    scan2000 = checked;
    ui->output->append(QString("%1 File scan %2").arg(2000).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk5000_toggled(bool checked)
{
    scan5000 = checked;
    ui->output->append(QString("%1 File scan %2").arg(5000).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chk10000_toggled(bool checked)
{
    scan10000 = checked;
    ui->output->append(QString("%1 File scan %2").arg(10000).arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::on_chkEnableDirectoryCreation_toggled(bool checked)
{
    this->enableDirectoryCreation = checked;
    ui->output->append(QString("Random directory creation for generic scan: %1").arg(checked ? "Enabled" : "Disabled"));
}

void MainWindow::createFiles(QDir rootDir, int numFiles, bool enableDirectoryCreation)
{
    QRandomGenerator random;
    QDir dir(rootDir);

    int dirLevel = 0;
    qsrand(0);
    int numFilesCreated = 0;
    while (numFiles > numFilesCreated) {
        numFilesCreated++;
        if(enableDirectoryCreation) {
            if(static_cast<int>(qrand() % 100) <= static_cast<int>(probDirCreated * 100)){
                QString dirName;
                dirName = QString::fromUtf8(QCryptographicHash::hash(QString::number(random.generate()).toUtf8(), QCryptographicHash::Sha256).toHex());
                dir.mkdir(dirName);
                dir.cd(dirName);
                dispatchToMainThread([this, dir](){
                   ui->output->append("Created Folder " + dir.path() + "");
                });
                dirLevel++;
                // create dir
                // cd
            } else {
                if((qrand() % 100) <= probGoBack * 100 && dirLevel > 0){
                    // cd up
                    dir.cdUp();
                    dirLevel--;
                }
                //else remiain in dir
            }
        }
        // create file
        QString fileName = QString::fromUtf8(QCryptographicHash::hash(QString::number(random.generate()).toUtf8(), QCryptographicHash::Sha256).toHex());
        QString filePath = dir.path() + "/" + fileName;
        dispatchToMainThread([this, filePath, numFilesCreated, numFiles](){
           ui->output->append(QString("(%1/%2) Created File %3").arg(numFilesCreated).arg(numFiles).arg(filePath));
        });
        QFile file(filePath);
        file.open(QIODevice::WriteOnly);
        QTextStream ts(&file);
        ts << fileName;
        file.close();
    }
}

void MainWindow::scan(const QDir &directory, QVector<int> numFilesScans, bool enableStorageRefresh, bool enableCrc)
{
    QStorageInfo storageInfo(directory);
    ui->lblDrive->setText(QString("Drive:\t%1").arg(storageInfo.rootPath()));

    int numFilesToScan = 0;
    for (int var = 0; var < numFilesScans.length(); ++var) {
        if(!running) {
            break;
        }
        numFilesToScan = numFilesScans.at(var);

        if(scanType == ScanType::GenericScan) {
            dispatchToMainThread([this, numFilesToScan]() {
               ui->output->append(QString("========================================\nStarting scan of %1 files").arg(numFilesToScan));
            });
        }

        QVector<qint64> fileScanTimes;
        QVector<qint64> crcTimes;
        QVector<qint64> storageRefreshTimes;
        fileScanTimes.clear();
        storageRefreshTimes.clear();
        crcTimes.clear();

        qint64 qTotalScanTime = 0, qTotalFileScanTime = 0, qTotalStorageRefreshTime = 0;

        QElapsedTimer fileScanTime;
        QElapsedTimer crcBuildTime;
        QElapsedTimer storageRefreshTime;
        QElapsedTimer totalScanTime;

        int scanned = 0;
        QDirIterator it(directory.path(), QDir::Files, QDirIterator::Subdirectories);

        totalScanTime.start();
        while (scanned < numFilesToScan && it.hasNext() && running) {
            fileScanTime.restart();
            // Output Scan Info
            QString filePath = it.next();
            QFileInfo fi(filePath);

            QString _1 = fi.completeBaseName(),
                    _2 = fi.completeSuffix(),
                    _3 = fi.metadataChangeTime().toString("yyyy-MM-dd_hh-mm-ss");
            if(verboseLogging) {
                dispatchToMainThread([this, _1, _2, _3, filePath](){
                    ui->output->append("Scanning " + filePath + "");
                    ui->output->append("\tFileName: " + _1 + "");
                    ui->output->append("\tExtension: "  + _2 + "");
                    ui->output->append("\tDate Modified: " + _3 + "");
                });
            }

            // Check CRC
            crcBuildTime.restart();
            if(enableCrc) {
                quint32 hash = crc32->calculateFromFile(filePath);
                QByteArray ba = QByteArray::number(hash);
                QString _1 = QString(ba.data());
                if(verboseLogging){
                    dispatchToMainThread([this, _1](){
                        ui->output->append("\tCRC: " + _1 + "");
                    });
                }
            }
            qint64 _crcBuildTime = crcBuildTime.elapsed();
            crcTimes.append(_crcBuildTime);

            // Stop file scanner
            qint64 _fileScanTime = fileScanTime.elapsed();
            fileScanTimes.append(_fileScanTime);


            // Check Storage
            storageRefreshTime.restart();
            if(enableStorageRefresh) {
                storageInfo.refresh();
                QString space = QString::number(100 - (storageInfo.bytesFree() / storageInfo.bytesTotal()));
                if(verboseLogging) {
                    dispatchToMainThread([this, space](){
                        ui->output->append("Storage Space : " + space + "% used");
                    });
                }
            }
            qint64 _storageRefreshTime = storageRefreshTime.elapsed();
            storageRefreshTimes.append(_storageRefreshTime);

            scanned++;
            dispatchToMainThread([this, storageRefreshTimes, fileScanTimes, scanned](){
                // determine average processing time
                ui->lblAverageStorageRefreshTime->setText(QString("Average Storage Refresh Time: %1ms").arg(std::accumulate(storageRefreshTimes.begin(), storageRefreshTimes.end(), 0) / storageRefreshTimes.length()));

                // determine average file scan time
                ui->lblAverageScanTime->setText(QString("Average Scan Time: %1ms").arg(std::accumulate(fileScanTimes.begin(), fileScanTimes.end(), 0) / fileScanTimes.length()));

                // update GUI
                ui->lblNumFilesScanned->setText(QString("# Files Scanned: %1").arg(scanned));
            });
        }

        // Stop total scan timer
        qTotalScanTime = totalScanTime.elapsed();
        qTotalFileScanTime = std::accumulate(fileScanTimes.begin(), fileScanTimes.end(), 0);
        qTotalStorageRefreshTime = std::accumulate(storageRefreshTimes.begin(), storageRefreshTimes.end(), 0);

        dispatchToMainThread([this, enableCrc, crcTimes, fileScanTimes, storageRefreshTimes, qTotalStorageRefreshTime, qTotalFileScanTime, qTotalScanTime](){
            ui->output->append(QString("Average File Scan Time: %1ms\nAverage Storage Refresh Time: %2ms").arg(std::accumulate(fileScanTimes.begin(), fileScanTimes.end(), 0) / fileScanTimes.length()).arg(std::accumulate(storageRefreshTimes.begin(), storageRefreshTimes.end(), 0) / storageRefreshTimes.length()));
            ui->output->append(QString("Total Time Refreshing Storage Info: %1ms\nTotal Time Scanning Files: %2ms").arg(qTotalStorageRefreshTime).arg(qTotalFileScanTime));
            if(enableCrc){
                auto localAccumulate = std::accumulate(crcTimes.begin(), crcTimes.end(), 0);
                ui->output->append(QString("Total Time Generating CRCs: %1ms").arg(localAccumulate));
            }
            ui->output->append(QString("Total Scanning Time: %1ms").arg(qTotalScanTime));
            ui->output->append(QString("========================================\n"));
        });
    }
}



void MainWindow::dispatchToMainThread(std::function<void()> callback)
{
    // any thread
    QTimer* timer = new QTimer();
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]()
    {
        // main thread
        callback();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

void MainWindow::startRunTimer()
{
    secondsPassed = 0;
    updateTimer = new QTimer;
    updateTimer->setInterval(1000);
    connect(updateTimer, &QTimer::timeout, [=](){
        secondsPassed++;
        ui->lblRunningTime->setText(QDateTime::fromTime_t(secondsPassed).toUTC().toString("hh:mm:ss"));
    });
    updateTimer->start();
}

void MainWindow::stopRunTimer()
{
    updateTimer->stop();
}

void MainWindow::analyzeDir()
{
    ui->btnStartStop->setEnabled(false);
    ui->output->append("=============================\n");
    ui->output->append(QString("Directory Selected: \n\t%1\n\nAnalyzing, please wait...").arg(selectedDir.path()));
    QThread::create([this](){
        QDirIterator it(selectedDir.path(), QDir::Files | QDir::Dirs, QDirIterator::Subdirectories);
        int files = 0;
        int dirs = 0;
        while (it.hasNext()) {
            QFileInfo f(it.next());
            if(f.isDir()) {
                dirs++;
            } else {
                files++;
            }
            this->numDirFiles = files;

        }
        dispatchToMainThread([this, dirs, files](){
            ui->output->append(QString("Number of folders: %1").arg(dirs));
            ui->output->append(QString("Number of files: %1").arg(files));
            ui->output->append("=============================\n");
            ui->btnStartStop->setEnabled(true);
        });
    })->start();

}

void MainWindow::runScan(QDir root, int numFiles)
{
    startRunTimer();
    // run scan
    ui->output->append("Scanning\n==============\n\n");
    ui->output->append(QString(" - Directory: %1\n - Number of Files: %2\n - Refresh Storage Info: %3\n - Enable CRC calculation: %4\n").arg(root.path()).arg(numFiles).arg(refreshOnScan ? "Yes" : "No").arg(calcCRC ? "Yes" : "No"));
    QVector<int> fileTests;
    if(scanType == ScanType::GenericScan) {
        if(scan50) fileTests.append(50);
        if(scan100) fileTests.append(100);
        if(scan200) fileTests.append(200);
        if(scan500) fileTests.append(500);
        if(scan1000) fileTests.append(1000);
        if(scan2000) fileTests.append(2000);
        if(scan5000) fileTests.append(5000);
        if(scan10000) fileTests.append(10000);
    } else {
        fileTests.append(numDirFiles);
    }

    QThread *thread = QThread::create([this, root, fileTests](){
        scan(root, fileTests, refreshOnScan, calcCRC);

        // Cleanup
        if(scanType == ScanType::GenericScan) {
            ui->output->append("Cleaning up\n===============\n");
            QDir rootDir(root);
            rootDir.removeRecursively();
            ui->output->append("done!");
        }
    });
    connect(thread, &QThread::finished, this, [this](){
        stopRunTimer();
        running = false;
        ui->btnStartStop->setText("Start");
    });
    thread->start();
}


void MainWindow::on_btnStartStop_clicked()
{
    if(running) {
        ui->output->append("Stopping scan...\n");
        running = false;
        return;
    }
    running = true;
    ui->output->clear();
    ui->btnStartStop->setText("Stop");
    int numFiles = INT_MAX;
    QDir root;
    if(scanType == ScanType::GenericScan) {
        root.setPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    } else {
        root.setPath(selectedDir.path());
    }


    // Create files
    if(scanType == ScanType::GenericScan) {
        ui->output->append("Creating files\n=========================\n");
        root.mkdir("StorageTester");
        root.cd("StorageTester");
        numFiles = scan10000 ? 10000 : scan5000 ? 5000 : scan2000 ? 2000 : scan1000 ? 1000 : scan500 ? 500 : scan200 ? 200 : scan100 ? 100 : scan50 ? 50 : 0;
        QThread *t = QThread::create([this, root, numFiles]() {
            // create files for max size
            createFiles(root, numFiles, enableDirectoryCreation);
        });
        connect(t, &QThread::finished, [=]() {
            runScan(root, numFiles);
        });
        t->start();

    } else {
        // run actual scan
        runScan(root, numFiles);
    }

}

void MainWindow::on_chkVerboseLogging_toggled(bool checked)
{
    this->verboseLogging = checked;
    ui->output->append(QString("Verbose Logging %1").arg(verboseLogging ? "enabled" : "disabled"));
}
