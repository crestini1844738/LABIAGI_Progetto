## LABIAGI_Progetto_Pick_And_Delivery

Input: 
   - mappa di un ambiente (diag)

Output: un sistema che gestisca il pick e  delivery da parte di un robot mobile

Comportamento:
   - Configurare uno stack di navigazione e simulazione con la mappa fornita
   - scrivere un server che controlla un robot mobile
     - il server fornisce un'interfaccia (anche testuale) ad utenti registrati,
       accessibile tramite un client
     - il server offre un servizio "pick" e "delivery" che permette ad un'utente
       in una stanza, di
     - chiamare un robot
     - una volta che il robot e' arrivato, l'utente mette un pacco sul robot
         e lo segnala al sistema mediante il client, selezionando un destinatario
     - il robot parte verso il destinatario ed attende da questo la conferma dell'
         avvenuta ricezione
     - il robot e' di nuovo libero per una nuova missione


