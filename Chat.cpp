#include <QMessageBox>
#include "Chat.h"

#include <iostream>

////////////////////////////////////////////////////////////////////////////////
// Chat ////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Processeurs.
const std::map<QString, Chat::Processor> Chat::PROCESSORS {
  {"#error", &Chat::process_error},
    {"#alias", &Chat::process_alias},
    {"#connected", &Chat::process_connected},
    {"#disconnected", &Chat::process_disconnected},
    {"#renamed", &Chat::process_renamed},
    {"#list", &Chat::process_list},
    {"#private", &Chat::process_private}
};

// Constructeur.
Chat::Chat (const QString & host, quint16 port, QObject * parent) :
  QObject (parent),
  socket ()
{

  // Signal "connected" émis lorsque la connexion est effectuée.
  // TODO
    connect(&socket, &QTcpSocket::connected, [this]{
        emit connected(socket.peerName(), socket.peerPort());
    });

  // Signale "disconnected" émis lors d'une déconnexion du socket.
  // TODO
    connect(&socket, &QTcpSocket::disconnected, this, &Chat::disconnected);
  // Lecture.
  connect (&socket, &QIODevice::readyRead, [this] () {
    // Tant que l'on peut lire une ligne...
    while (socket.canReadLine ())
    {
      // Lecture d'une ligne et suppression du "newline".
      QString m = socket.readLine ().chopped (1);

      // Flot de lecture.
      QTextStream stream (&m);
      // Lecture d'une commande potentielle.
      QString command;
      stream >> command;

      // Recherche de la commande serveur dans le tableau associatif.
      auto it = Chat::PROCESSORS.find(command);
      // - si elle existe, traitement du reste du message par le processeur ;
      if(it != PROCESSORS.end()) {
        (this->*it->second)(stream);
      }
      // - sinon, émission du signal "message" contenant la ligne entière.
      // TODO
      else {
        emit message(m);
      }
    }
  });

  // CONNEXION !
  socket.connectToHost (host, port, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);
}

Chat::~Chat ()
{
  // Déconnexion des signaux.
  socket.disconnect ();
}

// Commande "#alias"
// TODO
void Chat::process_alias(QTextStream &alias) {
    QString al;
    alias >> al >> Qt::ws;
    emit user_alias(al);
}
// Commande "#connected"
// TODO
void Chat::process_connected(QTextStream &alias) {
    QString al;
    alias >> al >> Qt::ws;
    emit user_connected(al);
}
// Commande "#disconnected"
// TODO
void Chat::process_disconnected(QTextStream &alias) {
    QString al;
    alias >> al >> Qt::ws;
    emit user_disconnected(al);
}
// Commande "#renamed"
// TODO
void Chat::process_renamed(QTextStream &alias) {
    QString al_old, al_new;
    alias >> al_old >> Qt::ws;
    alias >> al_new >> Qt::ws;
    emit user_renamed(al_old, al_new);
}
// Commande "#list"
// TODO
void Chat::process_list(QTextStream &list) {
    QStringList aliases;
    QString elm;
    while(!list.atEnd()) {
        list >> elm >> Qt::ws;
        aliases << elm;
        elm.clear();
    }
    emit user_list(aliases);
}
// Commande "#private"
// TODO
void Chat::process_private(QTextStream &is) {
    QString receiver, message;
    is >> receiver >> Qt::ws;
    message = is.readAll();
    emit user_private(receiver, message);
}
// Commande "#error"
void Chat::process_error (QTextStream & is)
{
  QString id;
  is >> id >> Qt::ws;
  emit error (id);
}

// Envoi d'un message à travers le socket.
void Chat::write (const QString & message)
{
  socket.write (message.toUtf8 () + '\n');
}

////////////////////////////////////////////////////////////////////////////////
// ChatWindow //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ChatWindow::ChatWindow (const QString & host, quint16 port, QWidget * parent) :
  QMainWindow (parent),
  chat (host, port, this),
  text (this),
  input (this)
{
  text.setReadOnly (true);
  setCentralWidget (&text);

  // Insertion du la zone de saisie.
  // QDockWidget insérable en haut ou en bas, inséré en bas.
  // TODO
  QDockWidget *inputZone = new QDockWidget("saisie");
  QFrame *frame = new QFrame(inputZone);
  QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
  layout->addWidget(&input);
  frame->setLayout(layout);
  inputZone->setWidget(frame);
  addDockWidget(Qt::BottomDockWidgetArea, inputZone);
  // Désactivation de la zone de saisie.
  input.setEnabled (false);

  // Liste des Utilisateurs
  QDockWidget *utilisateurs = new QDockWidget(tr("liste des utilisateurs"));
  listUtilisateurs = new QListWidget(this);
  utilisateurs->setWidget(listUtilisateurs);
  addDockWidget(Qt::RightDockWidgetArea, utilisateurs);

  // Envoi de messages lorsque la touche "entrée" est pressée.
  // - transmission du texte au moteur de messagerie instantanée ;
  // - effacement de la zone de saisie.
  // TODO
  connect(&input, &QLineEdit::returnPressed, [this]{
     chat.write(input.text());
     input.clear();
  });
  // Connexion.
  // - affichage d'un message confirmant la connexion ;
  // - saisie de l'alias ;
  // - envoi de l'alias ;
  // - activation de la zone de saisie.
  // TODO
  connect(&chat, &Chat::connected, [this]{
     QString alias = QInputDialog::getText(this, tr("connexion")
                                           , tr("alias :"));
     chat.write(alias);
     text.append("vous etes connecte");
     input.setEnabled(true);
  });
  // Déconnexion.
  // - désactivation de la zone de saisie.
  // - affichage d'un message pour signaler la déconnexion.
  // TODO
  connect(&chat, &Chat::disconnected, [this]{
      input.setEnabled(false);
      text.clear();
      text.append("vous venez d'etre deconnecte");
  });
  // Messages.
  connect (&chat, &Chat::message, [this] (const QString & message) {
    text.append (message);
  });

  // Liste des utilisateurs.
  connect(&chat, &Chat::user_list, [this] (const QStringList &list) {
     listUtilisateurs->clear();
     QString test_list;
     for(int i = 0; i < list.size(); ++i) {
         //test_list.append(list[i]);
         listUtilisateurs->addItem(list[i]);
     }
  });
  // Connexion d'un utilisateur.
  connect(&chat, &Chat::user_connected, [this] (const QString &c) {
      text.append(c + " vient de se connecter");
      listUtilisateurs->addItem(c);
  });
  // Déconnexion d'un utilisateur.
  connect(&chat, &Chat::user_disconnected, [this] (const QString &d) {
      text.append(d + " vient de se deconnecter");
      QList<QListWidgetItem *> item = listUtilisateurs->findItems(d, Qt::MatchExactly);
      delete item[0];
  });
  // Nouvel alias d'un utilisateur.
  connect(&chat, &Chat::user_renamed, [this] (const QString &old_al, const QString &al) {
      text.append(tr("changement de l'utilisateur de %1 en %2").arg(old_al).arg(al));
      QList<QListWidgetItem *> item = listUtilisateurs->findItems(old_al, Qt::MatchExactly);
      delete item.at(0);
      listUtilisateurs->addItem(al);
  });
  // Message privé.
  connect(&chat, &Chat::user_private, [this] (const QString &receiver, const QString &message) {
     text.append(tr("message de %1 : %2").arg(receiver).arg(message));
  });
  // TODO

  // Gestion des erreurs.
  connect (&chat, &Chat::error, [this] (const QString & id) {
    QMessageBox::critical (this, tr("Error"), id);
  });

  // CONNEXION !
  text.append (tr("<b>Connecting...</b>"));
}
