#include <QDebug>

#include "recordsmanager.h"

RecordsManager::RecordsManager(QObject *parent) :
    QObject(parent)
{
    _loaders = QVector<Downloader*>();
    for(int i = 0; i < PoolSize; ++i)
    {
        Downloader* loader = new Downloader(this);
        _loaders.push_back(loader);
        connect(loader, SIGNAL(downloaded(RecordInfo*)), SLOT(downloaded(RecordInfo*)));
        connect(loader, SIGNAL(downloadProgress(RecordInfo*,qint64,qint64)), SIGNAL(downloadProgress(RecordInfo*,qint64,qint64)));
    }
    _stub.setBusy();
}

RecordsManager::~RecordsManager()
{
//    for(auto record : _finishedRecords)
//        delete record;
}

void RecordsManager::addRecordsForDownloading(QVector<RecordInfo *> records)
{
    if(_records.isEmpty())
        _records.swap(records);
    else
        std::copy(records.begin(), records.end(), std::back_inserter(_records));
    hadMoreRecords();
}

void RecordsManager::addRecordForDownloading(RecordInfo *record)
{
    _records.push_back(record);
    hadMoreRecords();
}

void RecordsManager::downloaded(RecordInfo *rec)
{
    int index = _queuedRecords.indexOf(rec);
    if(index == -1)
    {
        qWarning() << "Unknown record: " << rec->title() << "\t\t" << rec->url();
        return;
    }

    _queuedRecords.removeAt(index);
    //emit recordFinished(rec);
    if(_records.size() > 0)
        hadMoreRecords();
}

void RecordsManager::hadMoreRecords()
{
    int queuedCount = _queuedRecords.size();
    if(queuedCount < PoolSize)
    {
        int delta = PoolSize - queuedCount;
        delta = qMin(delta, _records.size());
        while(delta > 0 && _records.size() > 0)
        {
            Downloader* loader = findFirstFreeDownloader();
            if(loader->isBusy())
            {
                //haven't had any free downloaders
                return;
            }
            auto record = _records.takeFirst();
            _queuedRecords.append(record);

            loader->get(record);
            delta--;
        }
    }
}

Downloader* RecordsManager::findFirstFreeDownloader()
{
    for(auto loader : _loaders)
    {
        if(!loader->isBusy())
            return loader;
    }
    return &_stub;
}
