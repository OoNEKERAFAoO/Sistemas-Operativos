#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <fcntl.h>
#include "funciones.h"
#include "lista.h"
#include "procesos.h"
#include "utilidades.h"
#include "malloc.h"
#include "mmap.h"
#include "mshared.h"

#define min(a, b) ((a<b) ? (a) : (b))

// Variables globales
int a; char b; double c;

// Muestra en pantalla el pid actual o el pid padre
void pid(char * parametro) {
   if (parametro == NULL) { printf("Pid actual: %d\n", getpid()); }
   else if (!strcmp(parametro, "-p")) { printf("Pid padre: %d\n", getppid()); }
   else { printf("Uso: pid [opciones]: Muestra el pid del proceso que ejecuta el shell.\n\nOpciones:\n  -p Muestra el pid del proceso padre del shell\n  -h Ayuda\n"); }
}

// Muestra los autores
void author() {

   printf("|-----------------------------------------------------------------|\n");
   printf("|                                                                 |\n");
   printf("|              Este shell ha sido realizado por:                  |\n");
   printf("|                                                                 |\n");
   printf("|     Rafael Alcalde Azpiazu (rafael.alcalde.azpizu@udc.es)       |\n");
   printf("|              Ivan Anta Porto (i.anta@udc.es)                    |\n");
   printf("|                                                                 |\n");
   printf("|-----------------------------------------------------------------|\n");

}

// Muestra el directorio actual
void getdir( char * cur_dir ) {
   if (getcwd(cur_dir, 2048) == NULL) {
      perror("Imposible obtener el directorio actual");
   } else {
      printf("Actualmente en: %s\n", cur_dir);
   }
}

// Cambia de directorio
void changedir( char * dir, char * cur_dir ) {
   int change;

   if (dir == NULL) {
      getdir(cur_dir);
   } else {
      change = chdir(dir);
      if (change == -1) {
         perror("Imposible cambiar el directorio");
      } else {
         getdir(cur_dir);
      }
   }
}

// Elimina un archivo
void removefile( char * file ) {
   struct stat file_info;
   if (file != NULL) {
      if (stat(file, &file_info) == -1) { perror("Imposible eliminar el fichero");
      } else {
         if(file_info.st_mode & S_IFDIR) {
            if(rmdir(file) == -1) { perror("Imposible eliminar el directorio");
            } else { printf("Directorio %s eliminado\n", file); }
         } else {
            if(unlink(file) == -1) { perror("Imposible eliminar el archivo");
            } else { printf("Archivo %s eliminado\n", file); }
         }
      }
   } else { printf("Uso: delete [archivo]: Elimina archivo. archivo es un fichero o un directorio vacio.\n");
   }
}

// Obtiene el tipo de fichero
char filetype(mode_t f_mode) {
   switch (f_mode & S_IFMT) {
      case S_IFSOCK:  return 's';
      case S_IFLNK:	return 'l';
      case S_IFREG:	return '-';
      case S_IFBLK:	return 'b';
      case S_IFDIR:	return 'd';
      case S_IFCHR:	return 'c';
      case S_IFIFO:	return 'p';
      default:		return '?';
   }
}

char * modoarchivo(mode_t f_mode) {
   static char permisos[12];
   strcpy(permisos, "---------- ");

   permisos[0] = filetype(f_mode);
   if (f_mode & S_IRUSR) permisos[1]='r';
   if (f_mode & S_IWUSR) permisos[2]='w';
   if (f_mode & S_IXUSR) permisos[3]='x';
   if (f_mode & S_IRGRP) permisos[4]='r';
   if (f_mode & S_IWGRP) permisos[5]='w';
   if (f_mode & S_IXGRP) permisos[6]='x';
   if (f_mode & S_IROTH) permisos[7]='r';
   if (f_mode & S_IWOTH) permisos[8]='w';
   if (f_mode & S_IXOTH) permisos[9]='x';
   if (f_mode & S_ISUID) permisos[3]='s';
   if (f_mode & S_ISGID) permisos[6]='s';
   if (f_mode & S_ISVTX) permisos[9]='t';
   return permisos;
}

// Obtiene los argumentos
int getarg(int argc, char * argv[], char ** path) {
   int flags = 0;
   int i;

   if (argc > 3) { printf("La función necesita 3 argumentos");
   } else {
      for (i = 0; i < argc ; ++i) {
         if(argv[i][0] == '-') {
            if(!strcmp(argv[i]+1, "a")) { flags |= HIDDEN_FILES; }
            else if(!strcmp(argv[i]+1, "s")) { flags |= SHORT_NAME; }
            else {
               printf("Uso: lista [opciones] [ruta]: Lista el directorio actual.\n\nOpciones:\n  -a Muestra los achivos ocultos\n  -s Muestra el nombre completo\n");
               flags = -1;
               break;
            }
         } else { *path = argv[i]; }
      }
   }
   return flags;
}

// Imprime información sobre el fichero
void print_fileinfo(struct stat file_info) {
   struct passwd * user_info = getpwuid(file_info.st_uid);;
   struct group * group_info = getgrgid(file_info.st_gid);
   struct tm * shora_fichero = (struct tm *) malloc(sizeof(struct tm));
   struct tm * shora_actual = (struct tm *) malloc(sizeof(struct tm));
   time_t thora_actual;
   char * hora_fichero = (char *) malloc(sizeof(char)*32);


   // Inodo del fichero, Modo de archivo y hard links del fichero
   printf("%10li %s %4li", file_info.st_ino, modoarchivo(file_info.st_mode), file_info.st_nlink);
   // Nombre del usuario
   if (user_info == NULL) { printf("%5i ", file_info.st_uid); }
   else { printf("%10s ", user_info->pw_name); }
   // Nombre del grupo
   if (group_info == NULL) { printf("%5i ", file_info.st_gid); }
   else { printf("%10s ", group_info->gr_name); }
   // Obtenemos la hora del sistema y del archivo
   time(&thora_actual);
   gmtime_r(&thora_actual, shora_actual);
   gmtime_r(&file_info.st_mtime, shora_fichero);
   if ((shora_actual->tm_year) == (shora_fichero->tm_year)) {
      strftime(hora_fichero, sizeof(char)*32, "%b %d %H:%M", shora_fichero);
   } else {
      strftime(hora_fichero, sizeof(char)*32, "%b %d %Y ", shora_fichero);
   }
   printf("%s ", hora_fichero);

   // Eliminamos los punteros reservados
   free(shora_fichero);
   free(shora_actual);
   free(hora_fichero);
}

// Imprime la dirección del link
void printslnk(char * path) {
   char * buf_slink = (char *) malloc(1024*sizeof(char));

   if(readlink(path, buf_slink, sizeof(buf_slink)) == -1) {
      printf("\n");
      perror("Fallo al seguir link simbólico");
   } else {
      printf(" -> %s\n", buf_slink);
   }

   free(buf_slink);
}

// Lista un directorio
void listdir( int argc, char * argv[] ) {
   int flags;
   char * dir_path = ".";
   DIR * pdir;
   struct dirent * sdir;

   if ((flags = getarg(argc, argv, &dir_path)) != -1) {
      if ((pdir = opendir(dir_path)) == NULL) {
         perror("Imposible listar el directorio");
      } else {
         char file_path[2048];
         struct stat file_info;

         while((sdir = readdir(pdir)) != NULL) {
            sprintf(file_path, "%s/%s", dir_path, sdir->d_name);
            if (lstat(file_path, &file_info) == -1) {
               perror("STAT: No se puede mostrar el archivo");
            } else {
               if (((sdir->d_name[0] != '.') && !(flags & HIDDEN_FILES)) || (flags & HIDDEN_FILES)) {
                  if (!(flags & SHORT_NAME)) { print_fileinfo(file_info); }
                  if(!(flags & SHORT_NAME) && (filetype(file_info.st_mode) == 'l')) {
                     printf("%s", sdir->d_name);
                     printslnk(file_path);
                  } else {
                     printf("%s\n", sdir->d_name);
                  }
               }
            }
          } // endwhile

          closedir(pdir);
      }
   }
}

// Elimina recursivamente directorios
void deltree(char * parametro){
   char path[1000];
   DIR * directorio;
   struct dirent * archivo;
   struct stat archivo_info;
   path[0]='\0';

   // Si no se le pasa ninguna ruta a la funcion
   if(parametro == NULL){
      printf("Error: hay que pasar un parametro");
   }else{
      // Si no se puede abrir el directorio
      if((directorio = opendir(parametro)) == NULL) {
         perror("Error: no se ha podido abrir el directorio: ");
      } else {
         // Se leen todas las entradas de directorio
         while((archivo = readdir(directorio))!=NULL) {
            // Se excluyen del procesado los directorios . y ..
            if(!strcmp(archivo->d_name, ".") || !strcmp(archivo->d_name, "..")) {
               continue;
            } else {
               sprintf(path,"%s%s%s", parametro, parametro[strlen(parametro)-1] == '/' ? "" : "/", archivo->d_name);
               // Se comprueba si hay acceso a la entrada de directorio
               if(stat(path,&archivo_info) == -1) {
                  printf("Imposible eliminar el directorio\n");
               } else {
                  // Si la entrada es un directorio se llama de nuevo a esta función
                  if(archivo_info.st_mode & S_IFDIR) {
                     deltree(path);
                  // Si no es un directorio se elimina el archivo
                  } else {
                     removefile(path);
                  }
               }
            }
         }
         // Finalmente borramos el directorio actual
         removefile(parametro);
         // Y cerramos el directorio
         closedir(directorio);
      }
   }
}

// Obtiene la prioridad
void getprioridad(char * parametro) {
   int prioridad;
   if (parametro == NULL) {
      if((prioridad = getpriority(PRIO_PROCESS, 0)) == -1)
         perror("Error al obtener la prioridad");
      else printf("Esta shell actual tiene prioridad %d\n", prioridad);
   } else {
      if((prioridad = getpriority(PRIO_PROCESS, atoi(parametro))) == -1)
         perror("Error al obtener la prioridad");
      else printf("El proceso %s tiene prioridad %i\n", parametro, prioridad);
   }
}

// Establece la prioridad
void setprioridad(int argc, char * argv[]) {
   if (argc < 2) getprioridad(argv[0]);
   else if (argc == 2) {
      if(setpriority(PRIO_PROCESS, atoi(argv[0]), atoi(argv[1])) == -1)
         perror("Error al mostrar la prioridad");
      else printf("Prioridad de %s cambiada a %s\n", argv[0], argv[1]);
   } else {
      printf("Uso: setpriority [pid] [valor]: Establece una prioridad.\n");
   }
}

// Crea un hijo y espera a que el hijo termine
void dofork() {
   int pid;
   if((pid = fork()) != 0){
      waitpid(pid, NULL, 0);
   }
}

// Ejecuta un programa sin crear un proceso nuevo
void execprog(char * argv[]) {
   if(argv[0] == NULL) printf("exec: Se necesita un argumento mínimo\n");
   else
      execvp(argv[0], argv); perror("Error al ejecutar"); exit(0);
}

// Ejecuta un programa sin crear un proceso nuevo
void execprogpri(int argc, char * argv[]) {
   if(argc < 2)
      printf("Uso: execpri [prioridad] [programa]: Ejecuta un programa con una prioridad\n");
   else {
      if (setpriority(PRIO_PROCESS, 0, atoi(argv[0])) == -1)
         perror("Error al establecer la prioridad");
      else execprog(argv+1);
   }
}

// Crea un proceso en primer plano
void primerplano(char * argv[]) {
   int pid;

   if(argv[0] == NULL) printf("pplano: Se necesita un argumento mínimo\n");
   if((pid = fork()) == 0) execprog(argv);
   else waitpid(pid, NULL, 0);
}

// Crea un proceso en primer plano con prioridad
void primerplanopri(int argc, char * argv[]) {
   int pid;

   if(argc < 2)
      printf("Uso: pplanopri [prioridad] [programa]: Ejecuta un programa en primer plano una prioridad\n");
   else {
      if((pid = fork()) == 0) {
         if (setpriority(PRIO_PROCESS, 0, atoi(argv[0])) == -1)
            perror("Error al establecer la prioridad");
         else execprog(argv+1);
      } else waitpid(pid, NULL, 0);
   }
}

// Crea un proceso en segundo plano
void segundoplano(char * argv[], lista l) {
   int pid;

   if(argv[0] == NULL) printf("splano: Se necesita un argumento mínimo\n");
   if((pid = fork()) == 0) execprog(argv);
   else insertarproceso(pid, argv, l);
}

// Crea un proceso en segundo plano con prioridad
void segundoplanopri(int argc, char *argv[],lista l){
   int pid;

   if(argc < 2)
      printf("Uso: splanopri [prioridad] [programa]: Ejecuta un programa en segundo plano una prioridad\n");
   else {
      if((pid = vfork()) == 0) {
         if (setpriority(PRIO_PROCESS, 0, atoi(argv[0])) == -1)
            perror("Error al establecer la prioridad");
         else execprog(argv+1);
      } else insertarproceso(pid, argv+1, l);
   }
}

void jobs_all(lista l){
   if(!esListaVacia(l)){
      datoproc * d;
      posicion p = primera(l);
      printf("%4s %4s %10s %6s %6s %s\n", "PID", "NICE", "TIME", "STATUS", "RETURN", "CMD");
      while((p != NULL)&&(!esfindelista(p, l)||(p == ultima (l)))) {
         actualizaproceso(p, l);
         d = getDato(p, l);
         mostrarproceso(d, l);
         p = siguiente(p, l);
      }
   } else {
      printf("No hay procesos en segundo plano\n");
   }
}

void jobs_filtrado (char *selector,lista l){
   int boolean = 1;

   if(!esListaVacia(l)){
      datoproc * d;
      posicion p = primera(l);
      printf("%4s %4s %10s %6s %6s %s\n", "PID", "NICE", "TIME", "STATUS", "RETURN", "CMD");
      while((p != NULL)&&(!esfindelista(p, l)||(p == ultima (l)))) {
         actualizaproceso(p, l);
         d = getDato(p, l);
         if(!strcmp(selector,d->status)){
            mostrarproceso(d, l);
            boolean = 0;
         }
         p = siguiente(p, l);
      }
   }

   if (boolean){
      printf("No hay procesos que mostrar\n");
   }
}

void jobs_pid(int pid, lista l){
   posicion p;
   datoproc * d;

   if(!esListaVacia(l)){
      if((p=buscardatop(pid,l))==NULL){
         printf("No se ha encontrado el proceso solicitado\n");
      } else {
         actualizaproceso(p, l);
         d = getDato(p, l);
         mostrarproceso(d, l);
      }
   }else{
      printf("No hay procesos que mostrar\n");
   }
}

// Muestra la lista de procesos en segundo plano
void jobs(int n,char * trozos[], lista l){
   int pid;

   if (n==0){
      jobs_all(l);
   } else if(!strcmp(trozos[0],"all")){
      jobs_all(l);
   } else if(!strcmp(trozos[0],"term")){
      jobs_filtrado(EXIT,l);
   } else if(!strcmp(trozos[0],"sig")){
      jobs_filtrado(SIGN,l);
   } else if (!strcmp(trozos[0],"stop")){
      jobs_filtrado(STOP,l);
   } else if(!strcmp(trozos[0],"act")){
      jobs_filtrado(ACT,l);
   } else if(((pid = atoi(trozos[0]))>0)&&(pid<INT_MAX)){
      jobs_pid(pid,l);
   } else {
      printf("Uso: jobs [parametro] [pid]: muestra el estado de procesos en segundo plano o el estado de uno con un pid concreto");
   }
}

// Limpia los procesos
void clearjobs(lista l){
   datoproc *d;
   posicion tmp;
   if(!esListaVacia(l)){
      posicion p = primera(l);
      while ((p != NULL)&&(!esfindelista(p, l)||(p == ultima (l)))){
         tmp = siguiente(p,l);
         actualizaproceso(p, l);
         d = getDato(p, l);
         if(!strcmp(d->status,SIGN)||!strcmp(d->status,EXIT)){
            eliminar(&eliminardatop,p,l);
         }
         p = tmp;
      }
   }
}

// Función recursiva
void recursiva(int n) {
   char automatico[512];
   static char estatico[512];

   printf("Parametro n = %d en %p\n", n, &n);
   printf("Array estatico en %p\n", estatico);
   printf("Array automatico en %p\n", automatico);

   if (n>0) recursiva(n-1);
}

// Llama a la función recursiva
void showrecursive(char * number) {
   if (number == NULL) printf("recursiva n\n");
   else recursiva(atoi(number));
}

void * atop(char * dir) {
   return (void *) strtoull(dir, NULL, 16);
}

void imprimeCaracter(char c) {
   if(c == '\n') printf("%2s ", "\\n");
   else if(c == '\t') printf("%2s ", "\\t");
   else if(c == '\r') printf("%2s ", "\\r");
   else if(isascii(c) && isprint(c)) printf("%2c ", c);
   else printf("%2s ", "");
}

void imprimeAscii(char c) {
    printf("%2x ", (unsigned char) c);
}

// Imprime el contenido de Memoria
void memdump(char* dir, char* count) {
   void *p; int i = 0; int j, max;
   if(dir == NULL) printf("memdump dir [count]\n");
   else {
      p = atop(dir);
      if (count == NULL) max = 25; else max = atoi(count);

      while (i<max) {
         for(j = i; j < min(i+25, max); j++) imprimeCaracter(*((char *) (p+j)));
         printf("\n");
         for(j = i; j < min(i+25, max); j++) imprimeAscii(*((char *) (p+j)));
         printf("\n");
         i += 25;
      }
   }
}
 /*
ssize_t LeerFichero(char * f, void *p, size_t cont)
{
   struct stat s;
   ssize_t n;
   int df;

   if(stat (f,&s)==-1 || ((df=open(f,O_RDONLY))=-1)) return -1;
   if(cont==-1) cont = s.st_size;
   if((n = read(df, p, cont)) == -1) return -1;
   close(df);
   return n;
}

void readfile(char * arg[]) {
   void *p;
   size_t cont=-1;
   ssize_t n;

   if (arg[0]==NULL || arg[1]==NULL)  printf("Faltan parametros");
   p = atop(arg[1]);
   if ((n=LeerFichero(arg[0], p, cont))==-1) perror("Error al leer el fichero");
   else printf("Leidos %ibytes en %p", n, &p);
}
*/

ssize_t leerFichero(char * f, void * p, size_t cont) {
   struct stat s; ssize_t n; int df;

   if(stat(f, &s) == -1 || ((df = open(f, O_RDONLY)) == -1)) return -1;
   if(cont == -1) cont = s.st_size;
   if((n = read(df, p, cont)) == -1) { close(df); return -1; }
   close(df);
   return n;
}

// Lee un fichero en una dirección
void readfile(char * argv[]) {
   void *p; ssize_t tam; size_t cont = -1;

   if(argv[0] == NULL || argv[1] == NULL) printf("readfile: faltan parametros\n");
   else {
      p = atop(argv[1]); if(argv[2] != NULL) cont = atoi(argv[2]);
      if((tam = leerFichero(argv[0], p, cont)) == -1)
         perror("Error al leer el fichero");
      else printf("Leidos %li bytes en %p\n", tam, p);
   }
}

ssize_t escribirFichero(char * f, void * p, size_t cont, int modo) {
   ssize_t n; int df;

   if((df = open(f, modo)) == -1) return -1;
   if((n = write(df, p, cont)) == -1) { close(df); return -1; }
   close(df);
   return n;
}

// Escribe a un fichero desde una dirección
void writefile(char * argv[]) {
   void *p; ssize_t tam; size_t cont; int modo = O_WRONLY | O_CREAT;

   if(argv[0] == NULL || argv[1] == NULL || argv[2] == NULL)
      printf("writefile: faltan parametros\n");
   else {
      p = atop(argv[1]); cont = atoi(argv[2]);
      if(argv[3] != NULL && !strcmp(argv[3], "-o")) modo |= O_TRUNC;
      if((tam = escribirFichero(argv[0], p, cont, modo)) == -1)
         perror("Error al escribir el fichero");
      else printf("Escritos %li bytes en %s\n", tam, argv[0]);
   }
}

// Muestra las credenciales
void MostrarCredenciales() {
   uid_t u = getuid();
   uid_t ue = geteuid();

   printf("Credencial real: %i\nCredencial efectiva: %i\n", u, ue);
}

// Cambia las credenciales
void changeuid(char * argv[]) {
   struct passwd *p;
   uid_t u;
   char mensaje [1024];

   if (argv[0]==NULL) { MostrarCredenciales(); return; }
   if (argv[1]!=NULL && strcmp(argv[0], "-l")) { printf ("uid [-l] uid\n"); return; }
   if (argv[1]==NULL)
      u=atoi(argv[0]); /*no hay primer parametro*/
   else if ((p=getpwnam(argv[1]))!=NULL) u=p->pw_uid;
        else { printf ("Imposible obtener informacion de login %s\n",argv[1]); return; }
   if (setuid(u)==-1) {
      sprintf(mensaje, "Error al intentar establecer la credencial %d", u);
      perror(mensaje);
   } else printf("Credencial cambiada a %d\n", u);
}

// Imprime direcciones de memoria
void showdir() {
   int local_a; char local_b; double local_c;

   printf("Funciones del programa:\n");
   printf("pid(): %p; getdir(): %p; jobs(): %p\n", &pid, &getdir, &jobs);
   printf("Variables globales:\n");
   printf("a: %p; b: %p; c: %p\n", &a, &b, &c);
   printf("Variables locales:\n");
   printf("local_a: %p; local_b: %p; local_c: %p\n", &local_a, &local_b, &local_c);
}

void showmallocs(lista l){
   if(!esListaVacia(l)){
      datomalloc * d;
      posicion p = primera(l);
      while((p != NULL)&&(!esfindelista(p, l)||(p == ultima (l)))) {
         d = getDato(p, l);
         mostrarmalloc(d, l);
         p = siguiente(p, l);
      }
   } else {
      printf("No hay memoria asignada\n");
   }
}

void deassignmalloc(size_t tamanno, lista l) {
   posicion p = buscardatomalloc(tamanno, l);
   if(p == NULL) printf("No se ha encontrado ninguna dirección con %li bytes\n", tamanno);
   else {
      datomalloc * d = getDato(p, l);
      printf("Desasignado %li de %p\n", tamanno, d->dir);
      eliminar(&eliminardatomalloc, p, l);
   }
}

void dommalloc(char* argv[], lista l) {
   if((argv[0] == NULL) || ((argv[1] == NULL) && !strcmp(argv[0], "-deassign")))
      showmallocs(l);
   else {
      if((argv[1] == NULL) && strcmp(argv[0], "-deassign")) {
         size_t tamanno = strtoull(argv[0], NULL, 10);
         insertarmalloc(tamanno, l);
      } else if((argv[2] == NULL) && !strcmp(argv[0], "-deassign")) {
         size_t tamanno = strtoull(argv[1], NULL, 10);
         deassignmalloc(tamanno, l);
      } else printf("malloc [-deassign] [tam]\n");
   }
}

void showmmaps(lista l){
   if(!esListaVacia(l)){
      datommap * d;
      posicion p = primera(l);
      while((p != NULL)&&(!esfindelista(p, l)||(p == ultima (l)))) {
         d = getDato(p, l);
         mostrarmmap(d, l);
         p = siguiente(p, l);
      }
   } else {
      printf("No hay ficheros mapeados\n");
   }
}

void deassignmmap(char * nombre, lista l) {
   posicion p = buscardatommap(nombre, l);
   if(p == NULL) printf("No se ha encontrado el archivo mapeado como %s\n", nombre);
   else {
      datommap * d = getDato(p, l);
      printf("Desasignado %s de %p\n", nombre, d->dir);
      eliminar(&eliminardatommap, p, l);
   }
}

void dommap(char* argv[], lista l) {
   if((argv[0] == NULL) || ((argv[1] == NULL) && !strcmp(argv[0], "-deassign")))
      showmmaps(l);
   else {
      if(!strcmp(argv[0], "-deassign")) deassignmmap(argv[1], l);
      else insertarmmap(argv[0], argv[1], l);
   }
}

void showmshared(lista l){
   if(!esListaVacia(l)){
      datomshared * d;
      posicion p = primera(l);
      while((p != NULL)&&(!esfindelista(p, l)||(p == ultima (l)))) {
         d = getDato(p, l);
         mostrarmshared(d, l);
         p = siguiente(p, l);
      }
   } else {
      printf("No hay memoria compartida mapeada\n");
   }
}

void deassignmshared(size_t tamanno, lista l) {
   posicion p = buscardatomshared(tamanno, l);
   if(p == NULL) printf("No se ha encontrado la memoria compartida de tamaño %d\n", tamanno);
   else {
      datomshared * d = getDato(p, l);
      printf("Desasignada zona de memoria de tamaño %d en %p\n", tamanno, d->dir);
      eliminar(&eliminardatomshared, p, l);
   }
}

void domshared(char*argv[],lista l){
   dir_t p;;
   if((argv[0] == NULL) || ((argv[1] == NULL) && !strcmp(argv[0], "-deassign")))
      showmshared(l);
   else {
      if(!strcmp(argv[0], "-deassign")) deassignmshared(atoi(argv[1]), l);
      else 
         if ((argv[1]!= NULL) && (p = (dir_t) (ObtenerMemoriaShmget(atoi(argv[0]),atoi(argv[1]),l))!=NULL)){
            printf("Memoria compartida de clave %d mapeada en %p\n", atoi(argv[1]), p);
         } else
         
         if ((p=ObtenerMemoriaShmget(atoi(argv[0]),NULL,l))==NULL){
            printf("No se ha podido encontrar la memoria compartida de clave %s\n",argv[0]);
         } else {
            printf("Memoria compartida de clave %s mapeada en %p\n", argv[0], p);
         }
   }   
}

void dodeassign(char * argv[] ,lista lalloc, lista lmap,lista lmshared){
   datomalloc  * auxloc;
   datommap    * auxmap;
   datomshared * auxshd;
   dir_t dir ;

   if (argv[0]!=NULL){
      dir = atop(argv[0]);
      if (!esListaVacia(lalloc)) {
         posicion p = primera(lalloc);
         auxloc = getDato(p,lalloc);
         while (!esfindelista(p, lalloc) && (auxloc->dir!=dir)){
            auxloc = getDato(p,lalloc);
            siguiente(p, lalloc);
         }
         if (auxloc->dir == dir) {
            eliminar(&eliminardatomalloc,p,lalloc);
            printf("Eliminada memoria de direccion %p asignada com mmalloc\n",dir);
            return;
         }
      }

      if (!esListaVacia(lmap)) {
         posicion p = primera(lmap);
         auxmap = getDato(p,lmap);
         while (!esfindelista(p, lmap) && (auxmap->dir!=dir)){
            auxmap = getDato(p,lmap);
            siguiente(p, lmap);
         }
         if (auxmap->dir == dir) {
            eliminar(&eliminardatommap,p,lmap);
            printf("Eliminada memoria de direccion %p asignada com mmap\n",dir);
            return;
         }
      }

      if (!esListaVacia(lmshared)) {
         posicion p = primera(lmshared);
         auxshd = getDato(p,lmshared);
         while (!esfindelista(p, lmshared) && (auxshd->dir!=dir)){
            auxshd = getDato(p,lmshared);
            siguiente(p, lmshared);
         }
         if (auxshd->dir == dir) {
            eliminar(&eliminardatomshared,p,lmshared);
            printf("Eliminada memoria de direccion %p asignada com mshared\n",dir);
            return;
         }
      }
      printf("No se ha encontrado la direccion de memoria %p\n",dir);
   } else {
      printf("Error : deassign precisa de un argumento \n");
   }
}

void dormkey (char * argv[]){   
   key_t clave;
   int id;
   char * key; 
   if (argv[0]==NULL){
      printf("Error: rmkey recibe un parametro \n");
      return;
   }
   
   key = argv[0];
   
   if (key==NULL || (clave=(key_t) strtoul(key,NULL,10))==IPC_PRIVATE){
      printf ("rmkey clave_valida\n");
      return;
   }
   if ((id=shmget(clave,0,0666))==-1){
      perror ("shmget: imposible obtener memoria compartida");
      return;
   }
   if (shmctl(id,IPC_RMID,NULL)==-1)
      perror ("shmctl: imposible eliminar memoria compartida\n");
   else 
      printf("Eliminada clave de memoria compartida %s \n",key);


}

void showmem(lista lalloc, lista lmap,lista lmshared) {
   showmallocs(lalloc); showmmaps(lmap); showmshared(lmshared);
}
