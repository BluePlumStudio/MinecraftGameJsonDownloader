#include <QtCore/qcoreapplication.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qsslconfiguration.h>
#include <qdebug.h>
#include <qeventloop.h>
#include <qdir.h>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QSslConfiguration config = QSslConfiguration::defaultConfiguration();
	config.setPeerVerifyMode(QSslSocket::VerifyNone);
	config.setProtocol(QSsl::TlsV1_2);

	QNetworkAccessManager manager;
	QNetworkReply * reply = nullptr;

	QFile * jsonFile = new QFile(QCoreApplication::applicationDirPath() + "/version_manifest.json");
	if (!jsonFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
	{
		qDebug() << "Failed to open" << jsonFile->fileName();
		return 1;
	}
	QNetworkRequest request;
	QEventLoop eventLoop;

	request.setSslConfiguration(config);
	request.setUrl(QUrl("https://launchermeta.mojang.com/mc/game/version_manifest.json"));

	reply = manager.get(request);
	QObject::connect(reply,
		static_cast<void(QNetworkReply::*)(qint64, qint64)>(&QNetworkReply::downloadProgress),
		[=](qint64 a, qint64 b)
	{
		jsonFile->write(reply->readAll());
	});

	QObject::connect(reply,
		&QNetworkReply::finished,
		[=]()
	{
		reply->deleteLater();

		qDebug() << jsonFile->fileName() << "finished!";

		jsonFile->flush();
	});

	QObject::connect(reply,
		&QNetworkReply::finished,
		&eventLoop,
		&QEventLoop::quit);

	eventLoop.exec();
	//
	jsonFile->seek(0);

	QJsonDocument doc = QJsonDocument::fromJson(jsonFile->readAll());
	QJsonObject obj = doc.object();
	QJsonArray array = obj.take("versions").toArray();

	QDir dir;
	dir.mkdir(QCoreApplication::applicationDirPath() + "/Jsons/");

	QFile * dataFile = new QFile(QCoreApplication::applicationDirPath() + "/Jsons/data.txt");
	if (!dataFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
	{
		qDebug() << "Failed to open" << jsonFile->fileName();
		return 1;
	}
	for (uint i = 0; i < array.size(); i++)
	{
		QJsonObject obj = array[i].toObject();
		/*
		if (obj.value("type").toString() == "release")
		{

		}
		*/
		QString url = obj.value("url").toString();
		QFileInfo fileInfo(url);
		QFile * file = new QFile(QCoreApplication::applicationDirPath() + "/Jsons/" + fileInfo.fileName());
		file->open(QIODevice::Truncate | QIODevice::ReadWrite);

		QNetworkRequest request;
		request.setSslConfiguration(config);
		request.setUrl(url);

		reply = manager.get(request);

		QObject::connect(reply,
			static_cast<void(QNetworkReply::*)(qint64, qint64)>(&QNetworkReply::downloadProgress),
			[=](qint64 a, qint64 b)
		{
			file->write(reply->readAll());
		});

		QObject::connect(reply,
			&QNetworkReply::finished,
			[=]()
		{
			reply->deleteLater();

			qDebug() << file->fileName() << "finished!";

			file->flush();
			file->seek(0);

			QJsonObject obj = QJsonDocument::fromJson(file->readAll()).object();
			QString data = file->fileName() + " minimumLauncherVersion:" + QString::number(obj.take("minimumLauncherVersion").toInt()) + "\n";
			dataFile->write(data.toLocal8Bit());
			dataFile->flush();

			file->close();
			file->deleteLater();

		});

	}

	return a.exec();
}
