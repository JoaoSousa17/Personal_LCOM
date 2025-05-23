#ifndef DICIONARIOS_H
#define DICIONARIOS_H

#define MAX_PALAVRAS 12
#define MAX_TAMANHO_PALAVRA 30
#define TOTAL_CATEGORIAS 25

typedef struct {
    char palavra[MAX_TAMANHO_PALAVRA];
    int pontuacao;
} Entrada;

typedef struct {
    char nome[50];
    char palavras[MAX_PALAVRAS][MAX_TAMANHO_PALAVRA];
    Entrada pontuacoes[MAX_PALAVRAS];
    int totalPalavras;
    int totalPontuacoes;
} Categoria;

static Categoria categorias[TOTAL_CATEGORIAS] = {
    {
        "Meses do Ano",
        {
            "janeiro", "fevereiro", "março", "abril", "maio", "junho",
            "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"
        },
        {
            {"janeiro",1}, {"fevereiro",2}, {"março",1}, {"abril",1},
            {"maio",2}, {"junho",1}, {"julho",2}, {"agosto",3},
            {"setembro",3}, {"outubro",3}, {"novembro",2}, {"dezembro",3}
        },
        12, 12
    },
    {
        "Cores",
        {
            "vermelho", "azul", "verde", "amarelo", "roxo", "laranja",
            "preto", "branco", "cinzento", "rosa", "castanho", "turquesa"
        },
        {
            {"vermelho",2}, {"azul",1}, {"verde",1}, {"amarelo",2},
            {"roxo",2}, {"laranja",2}, {"preto",1}, {"branco",1},
            {"cinzento",2}, {"rosa",1}, {"castanho",3}, {"turquesa",3}
        },
        12, 12
    },
    {
        "Frutas",
        {
            "maçã", "banana", "laranja", "melancia", "pera", "uva",
            "kiwi", "manga", "cereja", "ananás", "morango", "framboesa"
        },
        {
            {"maçã",1}, {"banana",1}, {"laranja",1}, {"melancia",3},
            {"pera",1}, {"uva",1}, {"kiwi",2}, {"manga",2},
            {"cereja",2}, {"ananás",2}, {"morango",2}, {"framboesa",3}
        },
        12, 12
    },
    {
        "Profissões",
        {
            "médico", "advogado", "professor", "engenheiro", "bombeiro", "polícia",
            "padeiro", "motorista", "arquiteto", "cientista", "dentista", "enfermeiro"
        },
        {
            {"médico",2}, {"advogado",3}, {"professor",2}, {"engenheiro",3},
            {"bombeiro",2}, {"polícia",2}, {"padeiro",1}, {"motorista",1},
            {"arquiteto",3}, {"cientista",3}, {"dentista",2}, {"enfermeiro",2}
        },
        12, 12
    },
    {
        "Espaço",
        {
            "lua", "sol", "terra", "marte", "júpiter", "saturno",
            "urano", "neptuno", "estrela", "galáxia", "cometa", "meteorito"
        },
        {
            {"lua",1}, {"sol",1}, {"terra",1}, {"marte",2},
            {"júpiter",2}, {"saturno",2}, {"urano",2}, {"neptuno",2},
            {"estrela",1}, {"galáxia",3}, {"cometa",2}, {"meteorito",3}
        },
        12, 12
    },
    {
        "Animais",
        {
            "cão", "gato", "elefante", "leão", "tigre", "urso",
            "girafa", "zebra", "rinoceronte", "hipopótamo", "lobo", "pinguim"
        },
        {
            {"cão",1}, {"gato",1}, {"elefante",3}, {"leão",2},
            {"tigre",2}, {"urso",2}, {"girafa",2}, {"zebra",1},
            {"rinoceronte",3}, {"hipopótamo",3}, {"lobo",1}, {"pinguim",2}
        },
        12, 12
    },
    {
        "Países Europeus",
        {
            "portugal", "espanha", "frança", "itália", "alemanha", "polónia",
            "grécia", "noruega", "suécia", "finlândia", "dinamarca", "roménia"
        },
        {
            {"portugal",1}, {"espanha",1}, {"frança",1}, {"itália",1},
            {"alemanha",2}, {"polónia",2}, {"grécia",1}, {"noruega",2},
            {"suécia",2}, {"finlândia",3}, {"dinamarca",2}, {"roménia",2}
        },
        12, 12
    },
    {
        "Instrumentos Musicais",
        {
            "guitarra", "piano", "violino", "bateria", "saxofone", "trompete",
            "flauta", "harpa", "acordeão", "clarinete", "tambor", "baixo"
        },
        {
            {"guitarra",2}, {"piano",2}, {"violino",3}, {"bateria",2},
            {"saxofone",3}, {"trompete",2}, {"flauta",1}, {"harpa",2},
            {"acordeão",3}, {"clarinete",3}, {"tambor",1}, {"baixo",1}
        },
        12, 12
    },
    {
        "Marcas de Carros",
        {
            "toyota", "bmw", "audi", "mercedes", "ford", "volkswagen",
            "renault", "peugeot", "honda", "fiat", "mazda", "kia"
        },
        {
            {"toyota",1}, {"bmw",2}, {"audi",2}, {"mercedes",2},
            {"ford",1}, {"volkswagen",2}, {"renault",1}, {"peugeot",1},
            {"honda",1}, {"fiat",1}, {"mazda",2}, {"kia",1}
        },
        12, 12
    },
    {
        "Cidades Portuguesas",
        {
            "lisboa", "porto", "coimbra", "braga", "aveiro", "faro",
            "guimarães", "viseu", "leiria", "évora", "setúbal", "cascais"
        },
        {
            {"lisboa",1}, {"porto",1}, {"coimbra",1}, {"braga",1},
            {"aveiro",1}, {"faro",1}, {"guimarães",2}, {"viseu",1},
            {"leiria",1}, {"évora",1}, {"setúbal",1}, {"cascais",1}
        },
        12, 12
    },
    {
        "Elementos Químicos",
        {
            "hidrogénio", "hélio", "lítio", "carbono", "oxigénio", "sódio",
            "magnésio", "enxofre", "ferro", "zinco", "urânio", "radônio"
        },
        {
            {"hidrogénio",2}, {"hélio",1}, {"lítio",2}, {"carbono",1},
            {"oxigénio",1}, {"sódio",2}, {"magnésio",3}, {"enxofre",2},
            {"ferro",1}, {"zinco",2}, {"urânio",3}, {"radônio",3}
        },
        12, 12
    },
    {
        "Planetas",
        {
            "mercúrio", "vénus", "terra", "marte", "júpiter", "saturno",
            "urano", "neptuno", "plutão", "ceres", "eris", "haumea"
        },
        {
            {"mercúrio",2}, {"vénus",1}, {"terra",1}, {"marte",2},
            {"júpiter",2}, {"saturno",2}, {"urano",2}, {"neptuno",2},
            {"plutão",2}, {"ceres",3}, {"eris",3}, {"haumea",3}
        },
        12, 12
    },
    {
        "Capitales do Mundo",
        {
            "lisboa", "paris", "roma", "berlim", "madrid", "oslo",
            "helsínquia", "ancara", "cabul", "teerão", "copenhaga", "bamaco"
        },
        {
            {"lisboa",1}, {"paris",1}, {"roma",1}, {"berlim",2},
            {"madrid",1}, {"oslo",2}, {"helsínquia",3}, {"ancara",2},
            {"cabul",2}, {"teerão",3}, {"copenhaga",3}, {"bamaco",3}
        },
        12, 12
    },
    {
        "Comidas Portuguesas",
        {
            "bacalhau", "cozido", "feijoada", "caldo verde", "arroz de pato", "tripas à moda do porto",
            "francesinha", "sardinhas", "pastel de nata", "leitão", "dobrada", "alheira"
        },
        {
            {"bacalhau",1}, {"cozido",1}, {"feijoada",2}, {"caldo verde",2},
            {"arroz de pato",3}, {"tripas à moda do porto",3}, {"francesinha",2}, {"sardinhas",1},
            {"pastel de nata",1}, {"leitão",2}, {"dobrada",2}, {"alheira",1}
        },
        12, 12
    },
    {
        "Ferramentas",
        {
            "martelo", "chave de fendas", "serrote", "alicate", "furadeira", "parafuso",
            "formão", "lima", "trena", "torno", "alavanca", "esmeril"
        },
        {
            {"martelo",1}, {"chave de fendas",2}, {"serrote",1}, {"alicate",1},
            {"furadeira",2}, {"parafuso",1}, {"formão",2}, {"lima",1},
            {"trena",2}, {"torno",2}, {"alavanca",3}, {"esmeril",3}
        },
        12, 12
    },
    {
        "Linguagens de Programação",
        {
            "c", "java", "python", "haskell", "javascript", "typescript",
            "rust", "go", "swift", "kotlin", "scala", "perl"
        },
        {
            {"c",1}, {"java",1}, {"python",1}, {"haskell",3},
            {"javascript",2}, {"typescript",3}, {"rust",2}, {"go",1},
            {"swift",2}, {"kotlin",2}, {"scala",2}, {"perl",1}
        },
        12, 12
    },
    {
        "Desportos",
        {
            "futebol", "basquetebol", "andebol", "voleibol", "rugby", "ténis",
            "golfe", "boxe", "natação", "esgrima", "ginástica", "karaté"
        },
        {
            {"futebol",1}, {"basquetebol",2}, {"andebol",2}, {"voleibol",2},
            {"rugby",2}, {"ténis",1}, {"golfe",1}, {"boxe",1},
            {"natação",1}, {"esgrima",3}, {"ginástica",2}, {"karaté",2}
        },
        12, 12
    },
    {
        "Oceanos e Mares",
        {
            "atlântico", "pacífico", "índico", "ártico", "antártico", "mediterrâneo",
            "mar morto", "mar báltico", "mar do norte", "mar vermelho", "mar de coral", "mar das caraíbas"
        },
        {
            {"atlântico",2}, {"pacífico",2}, {"índico",2}, {"ártico",2},
            {"antártico",3}, {"mediterrâneo",3}, {"mar morto",1}, {"mar báltico",2},
            {"mar do norte",2}, {"mar vermelho",2}, {"mar de coral",3}, {"mar das caraíbas",3}
        },
        12, 12
    },
    {
        "Corpos Humanos",
        {
            "coração", "cérebro", "pulmões", "estômago", "fígado", "rins",
            "intestino", "baço", "pâncreas", "esôfago", "traqueia", "tendões"
        },
        {
            {"coração",1}, {"cérebro",2}, {"pulmões",1}, {"estômago",1},
            {"fígado",1}, {"rins",1}, {"intestino",2}, {"baço",3},
            {"pâncreas",3}, {"esôfago",3}, {"traqueia",2}, {"tendões",2}
        },
        12, 12
    },
    {
        "Tecnologias",
        {
            "smartphone", "computador", "tablet", "telemóvel", "internet", "wi-fi",
            "bluetooth", "satélite", "drones", "realidade virtual", "inteligência artificial", "nuvem"
        },
        {
            {"smartphone",1}, {"computador",1}, {"tablet",1}, {"telemóvel",1},
            {"internet",1}, {"wi-fi",1}, {"bluetooth",2}, {"satélite",2},
            {"drones",1}, {"realidade virtual",3}, {"inteligência artificial",3}, {"nuvem",1}
        },
        12, 12
    },
    {
        "Reinos Animais",
        {
            "mamíferos", "répteis", "anfíbios", "peixes", "aves", "insetos",
            "aracnídeos", "moluscos", "crustáceos", "cnidários", "anelídeos", "equinodermos"
        },
        {
            {"mamíferos",1}, {"répteis",2}, {"anfíbios",2}, {"peixes",1},
            {"aves",1}, {"insetos",1}, {"aracnídeos",2}, {"moluscos",2},
            {"crustáceos",3}, {"cnidários",3}, {"anelídeos",3}, {"equinodermos",3}
        },
        12, 12
    },
    {
        "Festividades",
        {
            "natal", "páscoa", "ano novo", "carnaval", "halloween", "dia dos namorados",
            "dia do trabalhador", "dia da criança", "dia da mãe", "dia do pai", "são joão", "são martinho"
        },
        {
            {"natal",1}, {"páscoa",1}, {"ano novo",1}, {"carnaval",1},
            {"halloween",2}, {"dia dos namorados",2}, {"dia do trabalhador",2}, {"dia da criança",1},
            {"dia da mãe",1}, {"dia do pai",1}, {"são joão",1}, {"são martinho",2}
        },
        12, 12
    },
    {
        "Monumentos Mundiais",
        {
            "coliseu", "muralha da china", "cristo redentor", "machu picchu", "torre eiffel", "big ben",
            "taj mahal", "pirâmides do egito", "estátua da liberdade", "stonehenge", "petra", "torre de pisa"
        },
        {
            {"coliseu",1}, {"muralha da china",3}, {"cristo redentor",2}, {"machu picchu",3},
            {"torre eiffel",1}, {"big ben",1}, {"taj mahal",2}, {"pirâmides do egito",3},
            {"estátua da liberdade",2}, {"stonehenge",3}, {"petra",2}, {"torre de pisa",1}
        },
        12, 12
    },
    {
        "Ferramentas de Cozinha",
        {
            "faca", "garfo", "colher", "tacho", "frigideira", "escorredor",
            "batedeira", "ralador", "concha", "espátula", "descascador", "abre-latas"
        },
        {
            {"faca",1}, {"garfo",1}, {"colher",1}, {"tacho",1},
            {"frigideira",2}, {"escorredor",2}, {"batedeira",2}, {"ralador",2},
            {"concha",1}, {"espátula",1}, {"descascador",3}, {"abre-latas",2}
        },
        12, 12
    },
    {
        "Verbos em Português",
        {
            "ser", "estar", "ter", "ir", "fazer", "dizer",
            "poder", "ver", "dar", "saber", "querer", "chegar"
        },
        {
            {"ser",1}, {"estar",1}, {"ter",1}, {"ir",1},
            {"fazer",1}, {"dizer",2}, {"poder",1}, {"ver",1},
            {"dar",1}, {"saber",2}, {"querer",2}, {"chegar",1}
        },
        12, 12
    }    
};

// Quantidade total de categorias
static const int categoriasCount = TOTAL_CATEGORIAS;

#endif
