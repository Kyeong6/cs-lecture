from sqlalchemy import create_engine, text
from sqlalchemy.orm import declarative_base
from sqlalchemy.orm import sessionmaker

# 현재 본 서버 기준 패스워드(password)는 "Root1234!" 이므로 테스트 서버 패스워드로 맞춰야 한다.
password = "Root1234!"
db_url = f"mysql+mysqlconnector://root:{password}@localhost:3306/박경준"

# 데이터베이스 서버 주소를 이용한 연결 진행(Session 얻기)
engine = create_engine(db_url)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

Base = declarative_base()

# Dependency
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# 연결 확인 함수
def check_db_connection():
    try:
        # 연결 테스트용 쿼리 실행
        with engine.connect() as conn:
            # 간단한 쿼리 실행
            conn.execute(text("SELECT 1"))  
        print("데이터베이스 연결 성공")
    except Exception as e:
        print(f"데이터베이스 연결 실패: {e}")

# 테스트 실행
if __name__ == "__main__":
    check_db_connection()