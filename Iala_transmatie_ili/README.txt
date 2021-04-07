
	Arduino - Iala_translatie_ili

PINI:
	NUME			PORT		DESCRIERE
	cmd_door_1		2		comanda pt deschiderea usii 1 (in)
	cmd_door_2		3		comanda pt deschiderea usii 2 (in)
	isOpen1			4		senzor daca usa 1 este deschisa (in)
	isOpen2			5		senzor daca usa 2 este deschisa (in)

	open1			6		comanda de deschidere usa 1 (out)
	open2			7		comanda de deschidere usa 2 (out)
	door_1_led		8		led ce indica daca usa 1 este deschisa (out)
	door_2_led		9		led ce indica daca usa 2 este deschisa (out)
	simultaneous_cmd	10		led de eroare in caz ca se primesc doua comenzi simultane de deschidere a usilor (out)
	boath_open		11		led ce indica daca ambele usi sunt deschise (out)


Functii:
	
	check_doors() - verifica daca este vre-o usa deschisa di porneste led-urile corespunzatoare

	simultaneousERR() - pune pauza 3s functionarii sistemului si activeaza in acest timp portul "simultaneous_cmd" daca s-a primit o dubla comanda

	open_door(int out, int cmd, int cmd_err) - out = usa ce trebuie deschisa/ cmd = comanda pt deschiderea usii respective / cmd_err = comanda pt usa cealalta pt a verivica eroarea de dubla comanda / functia verifica daca a primit o comanda de deschidere valida si deschide usa respectiva

	loop() - nu face nimic daca este cel putin o usa deschisa altfel asteapta un semnal ca sa inceapa operatia deschiderii unei usi

Descrierea modului de operare:	
	Se activeaza toate iesirile pe LOW si trec in HIGH atunci cand dorim sa activam un led sau sa deschidem o usa. Functia loop() asteapta sa primeasca o comanda de a deschide o usa folosind functia "open_door(...)" care verifica primirea unei comenzi corecte atat din punct de vedere al duratei impulsului (500ms) cat si al suprapunerilor de comenzi (daca se petrece o suprapunere se aprinde led-ul de  dubla comanda "simultaneous_cmd" moment in care se intrerupe functionalitatea sistemului timp de 3s)