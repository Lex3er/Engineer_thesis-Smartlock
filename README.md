<h1> Kod źródłowy pracy dyplomowej "Projekt inteligentnego zamka do drzwi lokalu mieszkalnego z wykorzystaniem technologii bezprzewodowej i urządzenia mobilnego" </h1>
<hr/>
<h2> Kod źródłowy programu sterującego zamkiem inteligentnym wykonany w ramach pracy inżynierskiej na kierunku "Mechatronika" Politechniki Białostockiej (specjalizacja: "Konstrukcje inteligentne"). </h2>
<br/>
Repozytorium zawiera kod źródłowy zamka inteligentnego wykonany w języku C++ oraz HTML. Aplikacja mobilna sterująca zamkiem wykonana zaostała za pośrednictwem platformy MIT App Inventor
<br/>
<h2> Steszczenie </h2>
<hr/>
Praca dyplomowa opierała się na zaprojektowaniu, stworzeniu i zaprogramowaniu zamka inteligentnego wykorzystywanego w lokalach mieszkalnych i sterowanego za pomocą technologii bezprzewodowej i aplikacji mobilnej. Głównym założeniem było zaprojektowaniu konstrukcji którą można byłoby wykorzystać w zdalnym sterowaniu dostępem do lokali mieszkalnych z dodatkową możliwością manualnego dostępu do pomieszczeń w przypadku odcięcia zasilania sieciowego i awarii/rozładowania źródła zasilania dodatkowego. W tym celu wykorzystano układ z mikrokontrolerem ESP32 silnik krokowy Nema 17, sterownik TMC2226, potencjometr wieloobrotowy 10kOhm, keypad 4x4, diody LED, buzzer, kontaktron CMD1263 oraz inne elementy elektroniczne potrzebne do utworzenia źródła zasilania.
<br/>
Kod źródłowy został napisany w oparciu o potrzebę uzyskania odpowiedniego momentu obrotowego pozwalającego na przekręcenie wkładki bębenkowej zamka wpuszczanego w drzwi. Konstrukcja fizyczna zamka inteligentnego utworzona na podstawie druku 3D została zaopatrzona w przekładnię zębatą spowalaniającą, a program sterujący skonfigurowano tak, aby silnik krokowy był wstanie zmienić stan zamka w drzwiach (poszczególne obliczenia zostały zawarte w kodzie źródłowym na samym początku, w komentarzach).
